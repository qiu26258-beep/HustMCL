#include <hustmcl/services/file_acquirer.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QLockFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QProcess>
#include <QThread>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sddl.h>
#include <aclapi.h>
#include <cstdlib>
#include <iostream>
using namespace hustmcl;
using namespace hustmcl::application;
void check(bool value, const char* message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
struct Fake : FileAcquirer {
    Completion callback;
    bool cancelled = false;
    void acquire(const QString&, const QString&, const core::DownloadDescriptor&, Completion done) override { callback = std::move(done); }
    void cancel(const QString&) override { cancelled = true; }
};
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 3 && app.arguments()[1] == "--hold-lock") {
        QLockFile held(app.arguments()[2]);
        if (!held.tryLock()) return 2;
        std::cout << "locked\n" << std::flush;
        QThread::sleep(10);
        return 0;
    }
    Fake fake;
    FileAcquisition usecase(fake);
    check(usecase.start("root", {}), "start");
    auto old = fake.callback;
    const auto id = usecase.snapshot().taskId;
    check(!usecase.start("root", {}), "reject overlap");
    check(usecase.cancel(id) && fake.cancelled, "cancel request");
    check(usecase.snapshot().phase == AcquisitionPhase::Cancelling, "wait for cleanup");
    fake.callback({FileError::Cancelled, {}, false});
    check(usecase.snapshot().phase == AcquisitionPhase::Cancelled, "cancel acknowledged");
    check(usecase.start("root", {}), "retry");
    old({FileError::None, "old", false});
    check(usecase.snapshot().phase == AcquisitionPhase::Acquiring, "ignore old result");
    fake.callback({FileError::None, "cache", false});
    check(usecase.snapshot().phase == AcquisitionPhase::Cached, "cached");

    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost), "listen");
    QByteArray body("verified payload");
    QString mode = "ok";
    int requests = 0;
    QObject::connect(&server, &QTcpServer::newConnection, &app, [&] {
        while (auto* socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                socket->readAll();
                if (socket->property("answered").toBool()) return;
                socket->setProperty("answered", true);
                ++requests;
                if (mode == "stall") return;
                const auto payload = mode == "wrong" ? QByteArray(body.size(), 'x') : body;
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(mode == "truncated" ? payload.size()+10 : payload.size()) + "\r\nConnection: close\r\n\r\n" + payload);
                socket->disconnectFromHost();
            });
        }
    });
    core::DownloadDescriptor file{QString("http://127.0.0.1:%1/file").arg(server.serverPort()).toStdString(),
        QCryptographicHash::hash(body, QCryptographicHash::Sha1).toHex().toStdString(), static_cast<std::uint64_t>(body.size())};
    services::QtFileAcquirer service({250, true});
    auto run = [&](const QString& root, core::DownloadDescriptor input, bool cancel = false) {
        FileResult result;
        QEventLoop loop;
        bool completed = false;
        service.acquire("test", root, input, [&](FileResult value) { result = value; completed = true; loop.quit(); });
        if (cancel) QTimer::singleShot(30, &loop, [&] { service.cancel("test"); });
        if (!completed) loop.exec();
        check(QDir(root + "/staging").entryList(QDir::Files).isEmpty(), "temporary files cleaned");
        return result;
    };
    QTemporaryDir good;
    auto result = run(good.path(), file);
    check(result.error == FileError::None && !result.reused && QFile::exists(result.cachePath), "download publish");
    const auto count = requests;
    check(run(good.path(), file).reused && requests == count, "verified cache reuse");
    const auto native = QDir::toNativeSeparators(result.cachePath);
    HANDLE exclusive = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(exclusive != INVALID_HANDLE_VALUE, "exclusive handle");
    check(run(good.path(), file).error == FileError::Storage, "read permission failure");
    CloseHandle(exclusive);
    QFile corrupt(result.cachePath); check(corrupt.open(QIODevice::WriteOnly), "corrupt open"); corrupt.write("bad"); corrupt.close();
    check(run(good.path(), file).error == FileError::Integrity, "preserve corrupted cache");
    for (const auto& scenario : {QString("wrong"), QString("truncated"), QString("stall")}) {
        QTemporaryDir root;
        mode = scenario;
        const auto failure = run(root.path(), file);
        check(failure.error == (mode == "wrong" ? FileError::Integrity : mode == "stall" ? FileError::Timeout : FileError::Network), "fault classification");
        check(QDir(root.path()+"/cache/sha1").entryList(QDir::Files).isEmpty(), "no failed cache");
        mode = "ok";
        check(run(root.path(), file).error == FileError::None, "retry after failure");
    }
    QTemporaryDir cancelled;
    mode = "stall";
    check(run(cancelled.path(), file, true).error == FileError::Cancelled, "active cancellation");
    mode = "ok";
    check(run(cancelled.path(), file).error == FileError::None, "retry after cancel");
    QTemporaryDir locked;
    QLockFile lock(locked.path()+"/.write.lock"); check(lock.tryLock(), "hold lock");
    check(run(locked.path(), file).error == FileError::Locked, "lock conflict");
    QTemporaryDir processRoot;
    QProcess holder;
    holder.start(QCoreApplication::applicationFilePath(), {"--hold-lock", processRoot.path()+"/.write.lock"});
    check(holder.waitForStarted() && holder.waitForReadyRead(), "child owns lock");
    check(run(processRoot.path(), file).error == FileError::Locked, "cross-process conflict");
    holder.kill(); check(holder.waitForFinished(), "stop fixture child");
    QTemporaryDir wrongSize;
    auto small = file; small.size--;
    check(run(wrongSize.path(), small).error == FileError::Integrity, "oversize rejection");
    check(run("relative", file).error == FileError::UnsafePath, "relative rejected");
    auto invalid = file; invalid.sha1 = "../escape";
    check(run(wrongSize.path(), invalid).error == FileError::InvalidInput, "hash path rejected");
    QTemporaryDir blocked;
    QFile obstruction(blocked.path()+"/cache"); check(obstruction.open(QIODevice::WriteOnly), "obstruction"); obstruction.close();
    check(run(blocked.path(), file).error == FileError::UnsafePath, "non-directory rejected");
    {
        Fake late;
        auto* owner = new FileAcquisition(late);
        owner->start("root", file);
        delete owner;
        check(late.cancelled, "usecase destructor cancels");
        late.callback({FileError::Cancelled, {}, false});
    }
    {
        QTemporaryDir root;
        mode = "stall";
        bool callback = false;
        auto* dying = new services::QtFileAcquirer({1000, true});
        dying->acquire("dying", root.path(), file, [&](auto) { callback = true; });
        QEventLoop loop;
        QTimer::singleShot(40, &loop, &QEventLoop::quit);
        loop.exec();
        delete dying;
        QCoreApplication::processEvents();
        check(!callback && QDir(root.path()+"/staging").entryList(QDir::Files).isEmpty(), "destructor cleans without callback");
        mode = "ok";
        check(run(root.path(), file).error == FileError::None, "destructor releases lock");
    }
    {
        QTemporaryDir root, outside;
        const auto link = root.path()+"/cache";
        QProcess shell;
        shell.start("powershell.exe", {"-NoProfile", "-NonInteractive", "-Command",
            "New-Item -ItemType Junction -Path '"+link+"' -Target '"+outside.path()+"' | Out-Null"});
        check(shell.waitForFinished() && shell.exitCode() == 0, "create junction");
        const auto rejected = run(root.path(), file);
        const auto nativeLink = QDir::toNativeSeparators(link);
        check(RemoveDirectoryW(reinterpret_cast<LPCWSTR>(nativeLink.utf16())), "remove junction only");
        check(rejected.error == FileError::UnsafePath && QDir(outside.path()).entryList(QDir::NoDotAndDotDot|QDir::AllEntries).isEmpty(), "junction target untouched");
    }
    {
        QTemporaryDir root;
        QDir(root.path()).mkdir("staging");
        auto path = QDir::toNativeSeparators(root.path()+"/staging");
        auto* name = reinterpret_cast<LPWSTR>(path.data());
        PSECURITY_DESCRIPTOR original = nullptr, denied = nullptr;
        PACL originalAcl = nullptr, denyAcl = nullptr;
        check(GetNamedSecurityInfoW(name, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &originalAcl, nullptr, &original) == ERROR_SUCCESS, "read original ACL");
        check(ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:(D;;FW;;;WD)(A;;FA;;;WD)", SDDL_REVISION_1, &denied, nullptr), "build denied ACL");
        BOOL present = FALSE, defaulted = FALSE;
        check(GetSecurityDescriptorDacl(denied, &present, &denyAcl, &defaulted), "read deny ACL");
        check(SetNamedSecurityInfoW(name, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, denyAcl, nullptr) == ERROR_SUCCESS, "set test ACL");
        const auto failure = run(root.path(), file);
        const auto restored = SetNamedSecurityInfoW(name, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|UNPROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, originalAcl, nullptr);
        LocalFree(denied); LocalFree(original);
        check(restored == ERROR_SUCCESS, "restore test ACL");
        check(failure.error == FileError::Storage, "ACL write denial");
        check(run(root.path(), file).error == FileError::None, "retry after ACL restore");
    }
    std::cout << "file acquisition scenarios passed\n";
}
