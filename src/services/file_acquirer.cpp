#include <hustmcl/services/file_acquirer.h>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QDateTime>
#include <QSaveFile>
#include <limits>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace hustmcl::services {
namespace {
using application::FileError;
using application::FileResult;
struct OwnedTemporary {
    QString journal;
    ~OwnedTemporary() { if (!journal.isEmpty()) QFile::remove(journal); }
};
bool safeExistingPath(QString path) {
    path = QDir::cleanPath(path);
    for (;;) {
        const auto native = QDir::toNativeSeparators(path);
        const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()));
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        const QString parent = QFileInfo(path).absolutePath();
        if (parent == path) return true;
        path = parent;
    }
}
FileResult fetch(const QString& root, const core::DownloadDescriptor& descriptor,
                 QtFileAcquirer::Options options, const std::atomic_bool& cancelled,
                 QNetworkAccessManager* sharedManager = nullptr) {
    const QUrl url(QString::fromStdString(descriptor.url), QUrl::StrictMode);
    const QString hash = QString::fromStdString(descriptor.sha1);
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() || url.hasFragment() ||
        !(url.scheme() == "https" || (options.allowLoopbackHttp && url.scheme() == "http" && url.host() == "127.0.0.1")) ||
        !QRegularExpression("^[0-9a-f]{40}$").match(hash).hasMatch() ||
        descriptor.size > static_cast<std::uint64_t>(std::numeric_limits<qint64>::max()) || options.timeoutMs <= 0)
        return {FileError::InvalidInput, {}, false};
    if (!QRegularExpression("^[A-Za-z]:[/\\\\]").match(root).hasMatch() ||
        root.contains("..") || !QFileInfo(root).isDir() || !safeExistingPath(root))
        return {FileError::UnsafePath, {}, false};
    if (cancelled) return {FileError::Cancelled, {}, false};
    const QDir directory(root);
    const auto lockPath = directory.filePath(".write.lock");
    if (QFileInfo::exists(lockPath) && !safeExistingPath(lockPath)) return {FileError::UnsafePath, {}, false};
    QLockFile lock(lockPath);
    lock.setStaleLockTime(0);
    if (!sharedManager && !lock.tryLock(0)) return {FileError::Locked, {}, false};
    for (const auto& relative : {QString("cache"), QString("cache/sha1"), QString("staging")}) {
        const auto path = directory.filePath(relative);
        if (!QFileInfo::exists(path) && !directory.mkdir(relative)) return {FileError::Storage, {}, false};
        if (!QFileInfo(path).isDir() || !safeExistingPath(path)) return {FileError::UnsafePath, {}, false};
    }
    const auto destination = directory.filePath("cache/sha1/" + hash);
    if (QFileInfo::exists(destination)) {
        if (!safeExistingPath(destination)) return {FileError::UnsafePath, {}, false};
        QFile cached(destination);
        if (!cached.open(QIODevice::ReadOnly)) return {FileError::Storage, {}, false};
        if (cached.size() != static_cast<qint64>(descriptor.size)) return {FileError::Integrity, {}, false};
        QCryptographicHash digest(QCryptographicHash::Sha1);
        while (!cached.atEnd()) {
            if (cancelled) return {FileError::Cancelled, {}, false};
            const auto bytes = cached.read(65536);
            if (bytes.isEmpty() && cached.error() != QFileDevice::NoError) return {FileError::Storage, {}, false};
            digest.addData(bytes);
        }
        if (digest.result().toHex() != hash.toLatin1()) return {FileError::Integrity, {}, false};
        return {FileError::None, destination, true};
    }
    QTemporaryFile temporary(directory.filePath("staging/file-XXXXXX.part"));
    if (!temporary.open()) return {FileError::Storage, {}, false};
    OwnedTemporary ownership{temporary.fileName()+".owner"};
    QSaveFile journal(ownership.journal);
    const auto record = QByteArray("HustMCL-download-v1\n") + QFileInfo(temporary.fileName()).fileName().toUtf8();
    if (!journal.open(QIODevice::WriteOnly) || journal.write(record) != record.size() || !journal.commit()) return {FileError::Storage, {}, false};
    std::unique_ptr<QNetworkAccessManager> localManager;
    if (!sharedManager) { localManager = std::make_unique<QNetworkAccessManager>(); sharedManager = localManager.get(); }
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("Accept-Encoding", "identity");
    std::unique_ptr<QNetworkReply> ownedReply(sharedManager->get(request));
    auto* reply = ownedReply.get();
    reply->setReadBufferSize(65536);
    QEventLoop loop;
    QTimer deadline, cancellation;
    deadline.setSingleShot(true);
    FileError error = FileError::None;
    QCryptographicHash digest(QCryptographicHash::Sha1);
    std::uint64_t received = 0;
    auto drain = [&] {
        while (reply->bytesAvailable() && error == FileError::None) {
            const auto bytes = reply->read(65536);
            if (bytes.isEmpty()) break;
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status != 0 && status != 200) { reply->abort(); break; }
            const auto size = static_cast<std::uint64_t>(bytes.size());
            if (size > descriptor.size - received) { error = FileError::Integrity; reply->abort(); break; }
            if (temporary.write(bytes) != bytes.size()) { error = FileError::Storage; reply->abort(); break; }
            digest.addData(bytes);
            received += size;
        }
    };
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, drain);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&] { error = FileError::Timeout; reply->abort(); });
    QObject::connect(&cancellation, &QTimer::timeout, &loop, [&] {
        if (cancelled) { error = FileError::Cancelled; reply->abort(); }
    });
    deadline.start(options.timeoutMs);
    cancellation.start(10);
    loop.exec();
    drain();
    if (cancelled) return {FileError::Cancelled, {}, false};
    if (error != FileError::None) return {error, {}, false, error == FileError::Timeout};
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || status != 200) {
        const auto network = reply->error();
        bool retryable = status == 408 || status == 429 || status == 500 || status == 502 || status == 503 || status == 504;
        if (status == 0 || status == 200) retryable = network == QNetworkReply::RemoteHostClosedError || network == QNetworkReply::TimeoutError ||
            network == QNetworkReply::TemporaryNetworkFailureError || network == QNetworkReply::ConnectionRefusedError;
        int delay = 0;
        const auto value = reply->rawHeader("Retry-After");
        if (!value.isEmpty()) {
            bool ok = false; const auto seconds = value.toLongLong(&ok);
            qint64 milliseconds = 0;
            if (ok && seconds >= 0) milliseconds = seconds > 10 ? 10001 : seconds*1000;
            else {
                const auto date = QDateTime::fromString(QString::fromLatin1(value), Qt::RFC2822Date);
                if (!date.isValid()) retryable = false;
                else milliseconds = std::max<qint64>(0, QDateTime::currentDateTimeUtc().msecsTo(date));
            }
            if (milliseconds > 10000) retryable = false;
            else delay = static_cast<int>(milliseconds);
        }
        return {FileError::Network, {}, false, retryable, delay};
    }
    if (received != descriptor.size || digest.result().toHex() != hash.toLatin1()) return {FileError::Integrity, {}, false};
    if (!temporary.flush()) return {FileError::Storage, {}, false};
    temporary.close();
    if (cancelled) return {FileError::Cancelled, {}, false};
    if (!safeExistingPath(directory.filePath("cache/sha1")) || !safeExistingPath(directory.filePath("staging")))
        return {FileError::UnsafePath, {}, false};
    // QTemporaryFile::rename is atomic and never falls back to copying.
    if (!temporary.rename(destination))
        return {FileError::Storage, {}, false};
    temporary.setAutoRemove(false);
    return {FileError::None, destination, false};
}
FileResult fetchWithRetry(const QString& root, const core::DownloadDescriptor& file, QtFileAcquirer::Options options,
                          const std::atomic_bool& cancelled, QNetworkAccessManager* manager = nullptr) {
    FileResult result;
    for (int attempt = 0; attempt < 3; ++attempt) {
        result = fetch(root, file, options, cancelled, manager);
        if (!result.retryable || attempt == 2) return result;
        const int delay = std::max(250 << attempt, result.retryAfterMs);
        QEventLoop loop; QTimer finish, poll;
        finish.setSingleShot(true);
        QObject::connect(&finish, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] { if (cancelled) loop.quit(); });
        finish.start(delay); poll.start(10); loop.exec();
        if (cancelled) return {FileError::Cancelled, {}, false};
    }
    return result;
}
}
struct QtFileAcquirer::Session {
    std::shared_ptr<bool> alive = std::make_shared<bool>(true);
    struct Lane {
        QThread thread;
        QObject* context = nullptr;
        QNetworkAccessManager* manager = nullptr;
        QString operation;
        std::shared_ptr<std::atomic_bool> cancelled;
    };
    QString root;
    std::unique_ptr<QLockFile> lock;
    std::vector<std::unique_ptr<Lane>> lanes;
    ~Session() {
        *alive = false;
        for (auto& lane : lanes) if (lane->cancelled) *lane->cancelled = true;
        for (auto& lane : lanes) { lane->thread.quit(); lane->thread.wait(); }
    }
};
FileError QtFileAcquirer::beginSession(const QString& root, int concurrency) {
    if (worker_ || session_) return FileError::Busy;
    if (concurrency < 1 || concurrency > 4 || !QRegularExpression("^[A-Za-z]:[/\\\\]").match(root).hasMatch() ||
        root.contains("..") || !QFileInfo(root).isDir() || !safeExistingPath(root)) return FileError::UnsafePath;
    auto session = std::make_unique<Session>();
    session->root = root;
    const QDir directory(root);
    const auto lockPath = directory.filePath(".write.lock");
    if (QFileInfo::exists(lockPath) && !safeExistingPath(lockPath)) return FileError::UnsafePath;
    session->lock = std::make_unique<QLockFile>(lockPath);
    session->lock->setStaleLockTime(0);
    if (!session->lock->tryLock()) return FileError::Locked;
    for (const auto& relative : {QString("cache"), QString("cache/sha1"), QString("staging")}) {
        const auto path = directory.filePath(relative);
        if (!QFileInfo::exists(path) && !directory.mkdir(relative)) return FileError::Storage;
        if (!QFileInfo(path).isDir() || !safeExistingPath(path)) return FileError::UnsafePath;
    }
    // Only exact journal/part pairs owned by this downloader, under the locked root.
    const QDir staging(directory.filePath("staging"));
    for (const auto& name : staging.entryList({"file-*.part.owner"}, QDir::Files)) {
        if (!QRegularExpression("^file-[A-Za-z0-9]{6}\\.part\\.owner$").match(name).hasMatch()) continue;
        const auto marker = staging.filePath(name), part = marker.chopped(6);
        if (!safeExistingPath(marker) || (QFileInfo::exists(part) && !safeExistingPath(part))) return FileError::UnsafePath;
        QFile journal(marker);
        if (!journal.open(QIODevice::ReadOnly) || journal.size() > 128) continue;
        const auto expected = QByteArray("HustMCL-download-v1\n")+QFileInfo(part).fileName().toUtf8();
        const bool owned = journal.readAll() == expected; journal.close();
        if (!owned) continue;
        if (QFileInfo::exists(part) && !QFile::remove(part)) return FileError::Storage;
        if (!QFile::remove(marker)) return FileError::Storage;
    }
    for (int i = 0; i < concurrency; ++i) {
        auto lane = std::make_unique<Session::Lane>();
        lane->context = new QObject;
        lane->context->moveToThread(&lane->thread);
        connect(&lane->thread, &QThread::finished, lane->context, &QObject::deleteLater);
        lane->thread.start();
        session->lanes.push_back(std::move(lane));
    }
    session_ = std::move(session);
    return FileError::None;
}
void QtFileAcquirer::endSession() { session_.reset(); }
QtFileAcquirer::QtFileAcquirer(Options options, QObject* parent) : QObject(parent), options_(options) {}
QtFileAcquirer::~QtFileAcquirer() {
    session_.reset();
    if (cancelled_) *cancelled_ = true;
    if (worker_) { worker_->wait(); delete worker_; }
}
void QtFileAcquirer::acquire(const QString& id, const QString& root, const core::DownloadDescriptor& file, Completion complete) {
    if (session_) {
        if (root != session_->root) { complete({FileError::InvalidInput, {}, false}); return; }
        for (auto& entry : session_->lanes) {
            auto* lane = entry.get();
            if (!lane->operation.isEmpty()) continue;
            lane->operation = id;
            lane->cancelled = std::make_shared<std::atomic_bool>(false);
            QMetaObject::invokeMethod(lane->context, [this, lane, root, file, complete = std::move(complete), flag = lane->cancelled, alive = session_->alive] {
                if (!lane->manager) lane->manager = new QNetworkAccessManager(lane->context);
                auto result = fetchWithRetry(root, file, options_, *flag, lane->manager);
                QMetaObject::invokeMethod(this, [lane, result, complete, alive] {
                    if (!*alive) return;
                    lane->operation.clear(); lane->cancelled.reset(); complete(result);
                }, Qt::QueuedConnection);
            }, Qt::QueuedConnection);
            return;
        }
        complete({FileError::Busy, {}, false}); return;
    }
    if (worker_) { complete({FileError::Busy, {}, false}); return; }
    taskId_ = id;
    cancelled_ = std::make_shared<std::atomic_bool>(false);
    auto result = std::make_shared<FileResult>();
    worker_ = QThread::create([root, file, options = options_, flag = cancelled_, result] { *result = fetch(root, file, options, *flag); });
    connect(worker_, &QThread::finished, this, [this, result, complete = std::move(complete)] {
        worker_->wait();
        delete worker_;
        worker_ = nullptr;
        cancelled_.reset();
        taskId_.clear();
        complete(*result);
    });
    worker_->start();
}
void QtFileAcquirer::cancel(const QString& id) {
    if (session_) for (auto& lane : session_->lanes) if (lane->operation == id && lane->cancelled) *lane->cancelled = true;
    if (cancelled_ && id == taskId_) *cancelled_ = true;
}
}
