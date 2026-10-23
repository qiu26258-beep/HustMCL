#include <hustmcl/application/batch_acquisition.h>
#include <hustmcl/services/file_acquirer.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QLockFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QFile>
#include <QElapsedTimer>
#include <cstdlib>
#include <iostream>
using namespace hustmcl;
using namespace hustmcl::application;
void check(bool ok, const char* message) { if (!ok) { std::cerr << message << '\n'; std::abort(); } }
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTcpServer server; check(server.listen(QHostAddress::LocalHost), "listen");
    int connections = 0, active = 0, peak = 0, requests = 0;
    bool stall = false;
    int httpStatus = 0, failuresLeft = 0;
    QByteArray retryAfter;
    QObject::connect(&server, &QTcpServer::newConnection, &app, [&] {
        while (auto* socket = server.nextPendingConnection()) {
            ++connections;
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                auto buffer = socket->property("buffer").toByteArray() + socket->readAll();
                if (!buffer.contains("\r\n\r\n")) { socket->setProperty("buffer", buffer); return; }
                socket->setProperty("buffer", QByteArray{});
                const auto body = buffer.split(' ').value(1);
                ++requests; ++active; peak = std::max(peak, active);
                if (httpStatus && failuresLeft-- > 0) {
                    --active;
                    socket->write("HTTP/1.1 " + QByteArray::number(httpStatus) + " Failure\r\nContent-Length: 0\r\nConnection: keep-alive\r\n" +
                        (retryAfter.isEmpty() ? QByteArray{} : "Retry-After: " + retryAfter + "\r\n") + "\r\n");
                    return;
                }
                if (stall) return;
                QTimer::singleShot(30, socket, [&, socket, body] {
                    --active;
                    socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: keep-alive\r\n\r\n" + body);
                });
            });
        }
    });
    core::InstallFilePlan plan;
    for (int i = 0; i < 12; ++i) {
        const auto body = QByteArray("/file-")+QByteArray::number(i);
        core::DownloadDescriptor d{QString("http://127.0.0.1:%1").arg(server.serverPort()).toStdString()+body.toStdString(),
            QCryptographicHash::hash(body, QCryptographicHash::Sha1).toHex().toStdString(), static_cast<std::uint64_t>(body.size())};
        plan.files.push_back({std::to_string(i), d, core::FileRole::Library}); plan.totalBytes += d.size;
    }
    QTemporaryDir root;
    services::QtFileAcquirer service({1000, true});
    BatchAcquisition batch(service, nullptr, 4);
    auto run = [&](bool cancel) {
        QEventLoop loop;
        auto connection = QObject::connect(&batch, &BatchAcquisition::changed, &loop, [&](auto state) {
            if (state.phase == AcquisitionPhase::Cached || state.phase == AcquisitionPhase::Cancelled || state.phase == AcquisitionPhase::Failed) loop.quit();
        });
        batch.start(root.path(), plan);
        QLockFile competitor(root.path()+"/.write.lock"); check(!competitor.tryLock(), "batch owns lock before dispatch");
        if (cancel) QTimer::singleShot(60, &loop, [&] { batch.cancel(batch.snapshot().taskId); });
        loop.exec(); QObject::disconnect(connection);
        check(competitor.tryLock(), "batch releases lock after cleanup");
    };
    run(false);
    check(batch.snapshot().phase == AcquisitionPhase::Cached, "batch cached");
    check(peak == 4 && connections <= 4 && requests == 12, "bounded parallelism and keep-alive reuse");
    const auto before = requests; run(false);
    check(batch.snapshot().reusedFiles == 12 && requests == before, "batch cache reuse");
    for (auto& file : plan.files) { file.download.url += "x"; file.download.sha1 = QCryptographicHash::hash(QByteArray::fromStdString(file.download.url), QCryptographicHash::Sha1).toHex().toStdString(); file.download.size = 1; }
    plan.totalBytes = plan.files.size(); stall = true;
    run(true);
    check(batch.snapshot().phase == AcquisitionPhase::Cancelled, "cancel acknowledged");
    check(QDir(root.path()+"/staging").entryList(QDir::Files).isEmpty(), "cancel cleaned all staging");
    stall = false; active = 0;
    const QByteArray payload("/retry");
    core::DownloadDescriptor retryFile{QString("http://127.0.0.1:%1/retry").arg(server.serverPort()).toStdString(),
        QCryptographicHash::hash(payload, QCryptographicHash::Sha1).toHex().toStdString(), static_cast<std::uint64_t>(payload.size())};
    auto retryRun = [&](int status, int failures, QByteArray after, bool cancel) {
        QTemporaryDir temp; httpStatus = status; failuresLeft = failures; retryAfter = after;
        check(service.beginSession(temp.path(), 1) == FileError::None, "retry session");
        QEventLoop loop; FileResult result;
        service.acquire("retry", temp.path(), retryFile, [&](auto value) { result = value; loop.quit(); });
        if (cancel) QTimer::singleShot(50, &loop, [&] { service.cancel("retry"); });
        loop.exec(); service.endSession();
        check(QDir(temp.path()+"/staging").entryList(QDir::Files).isEmpty(), "retry staging clean");
        return result;
    };
    int initial = requests;
    check(retryRun(503, 2, {}, false).error == FileError::None && requests-initial == 3, "transient retry success");
    initial = requests;
    check(retryRun(503, 9, {}, false).error == FileError::Network && requests-initial == 3, "retry bounded");
    initial = requests;
    check(retryRun(404, 9, {}, false).error == FileError::Network && requests-initial == 1, "permanent no retry");
    initial = requests;
    check(retryRun(429, 9, "60", false).error == FileError::Network && requests-initial == 1, "long retry-after not violated");
    initial = requests;
    check(retryRun(429, 9, "1", true).error == FileError::Cancelled && requests-initial == 1, "cancel during backoff");
    QElapsedTimer elapsed; elapsed.start();
    check(retryRun(429, 1, "1", false).error == FileError::None && elapsed.elapsed() >= 950, "honor retry-after");
    {
        QTemporaryDir recovery; QDir(recovery.path()).mkdir("staging");
        auto write = [&](QString name, QByteArray value) { QFile f(recovery.path()+"/staging/"+name); check(f.open(QIODevice::WriteOnly), "fixture open"); f.write(value); };
        write("file-ABC123.part", "orphan");
        write("file-ABC123.part.owner", "HustMCL-download-v1\nfile-ABC123.part");
        write("file-XYZ123.part", "unowned");
        check(service.beginSession(recovery.path(), 1) == FileError::None, "recover session");
        check(!QFile::exists(recovery.path()+"/staging/file-ABC123.part") && QFile::exists(recovery.path()+"/staging/file-XYZ123.part"), "recover only owned pair");
        service.endSession();
    }
    std::cout << "session concurrency and reuse passed\n";
}
