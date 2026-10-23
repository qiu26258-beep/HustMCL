#include <hustmcl/services/content_download_store.h>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QPointer>
#include <QTemporaryFile>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace hustmcl::services {
using namespace application;
namespace {
bool safe(QString path) {
    path=QDir::cleanPath(path);
    if (!QDir::isAbsolutePath(path)) return false;
    for (;;) {
        const auto native=QDir::toNativeSeparators(path);
        const auto attributes=GetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()));
        if (attributes==INVALID_FILE_ATTRIBUTES || (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        const auto parent=QFileInfo(path).absolutePath(); if (parent==path) return true; path=parent;
    }
}
ContentError verify(const QString& path,const core::ContentFile& file,const std::atomic_bool& cancelled) {
    if (!safe(path) || !QFileInfo(path).isFile()) return ContentError::Storage;
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) return ContentError::Storage;
    if (input.size()!=static_cast<qint64>(file.download.size)) return ContentError::Integrity;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    while (!input.atEnd()) {
        if(cancelled) return ContentError::Cancelled;
        const auto bytes=input.read(65536);
        if(bytes.isEmpty() && input.error()!=QFile::NoError) return ContentError::Storage;
        hash.addData(bytes);
    }
    return hash.result().toHex().toStdString()==file.download.sha1 ? ContentError::None : ContentError::Integrity;
}
ContentResult<ContentDownloadResult> publish(const QString& root,const QString& cache,const ContentDownloadRequest& request,const std::atomic_bool& cancelled) {
    if (!safe(root)) return ContentError::Storage;
    const auto verified=verify(cache,request.file,cancelled); if(verified!=ContentError::None) return verified;
    QString directory=root;
    for (const auto& part : {QString("content-downloads"),QString("modrinth"),QString::fromStdString(request.projectId),QString::fromStdString(request.versionId)}) {
        const auto next=QDir(directory).filePath(part);
        if(!QFileInfo::exists(next) && !QDir(directory).mkdir(part)) return ContentError::Storage;
        if(!safe(next) || !QFileInfo(next).isDir()) return ContentError::Storage;
        directory=next;
    }
    const auto lockPath=QDir(root).filePath("content-downloads/.publish.lock");
    if(QFileInfo::exists(lockPath) && !safe(lockPath)) return ContentError::Storage;
    QLockFile lock(lockPath); lock.setStaleLockTime(0);
    if(!lock.tryLock(0)) return ContentError::Conflict;
    const auto target=QDir(directory).filePath(QString::fromStdString(request.file.name));
    if(QFileInfo::exists(target)) {
        const auto current=verify(target,request.file,cancelled);
        if(current==ContentError::Cancelled) return current;
        if(current!=ContentError::None) return ContentError::Conflict;
        return ContentDownloadResult{target,true};
    }
    QTemporaryFile staging(QDir(directory).filePath(".hustmcl-XXXXXX.part"));
    QFile input(cache);
    if(!staging.open() || !input.open(QIODevice::ReadOnly)) return ContentError::Storage;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    qint64 size=0;
    while(!input.atEnd()) {
        if(cancelled) return ContentError::Cancelled;
        const auto bytes=input.read(65536);
        if(bytes.isEmpty() && input.error()!=QFile::NoError) return ContentError::Storage;
        size+=bytes.size(); hash.addData(bytes);
        if(size>static_cast<qint64>(request.file.download.size) || staging.write(bytes)!=bytes.size()) return ContentError::Storage;
    }
    if(size!=static_cast<qint64>(request.file.download.size) || hash.result().toHex().toStdString()!=request.file.download.sha1) return ContentError::Integrity;
    if(cancelled) return ContentError::Cancelled;
    if(!staging.flush() || !safe(directory)) return ContentError::Storage;
    staging.close();
    // Same-volume rename; QFile never overwrites an existing destination.
    if(!staging.rename(target)) return ContentError::Conflict;
    staging.setAutoRemove(false);
    return ContentDownloadResult{target,false};
}
ContentError fileError(FileError error) {
    switch(error) {
    case FileError::Cancelled:return ContentError::Cancelled;
    case FileError::Integrity:return ContentError::Integrity;
    case FileError::Network:return ContentError::Network;
    case FileError::Timeout:return ContentError::Timeout;
    case FileError::Locked:case FileError::Busy:return ContentError::Conflict;
    default:return ContentError::Storage;
    }
}
}
QtContentDownloadStore::QtContentDownloadStore(QString root,FileAcquirer& acquirer,QObject* parent)
    : QObject(parent), root_(std::move(root)), acquirer_(acquirer) { workers_.setMaxThreadCount(1); }
QtContentDownloadStore::~QtContentDownloadStore() {
    const auto id=operation_; operation_.clear(); if(cancelled_) *cancelled_=true;
    acquirer_.cancel(id); workers_.waitForDone();
}
void QtContentDownloadStore::acquire(const QString& id,const ContentDownloadRequest& request,std::function<void(ContentResult<ContentDownloadResult>)> done) {
    if(!operation_.isEmpty()) { done(ContentError::Conflict); return; }
    if(id.isEmpty() || !core::validContentId(request.projectId) || !core::validContentId(request.versionId) || !core::validContentFile(request.file)) { done(ContentError::InvalidRequest); return; }
    operation_=id; cancelled_=std::make_shared<std::atomic_bool>(false);
    const QPointer<QtContentDownloadStore> guard(this);
    acquirer_.acquire(id,root_,request.file.download,[guard,id,request,done=std::move(done)](FileResult result) mutable {
        if(!guard || guard->operation_!=id) return;
        if(result.error!=FileError::None) { guard->operation_.clear(); guard->cancelled_.reset(); done(fileError(result.error)); return; }
        const auto root=guard->root_; const auto cancelled=guard->cancelled_;
        guard->workers_.start([guard,id,root,request,result,cancelled,done=std::move(done)]() mutable {
            auto output=publish(root,result.cachePath,request,*cancelled);
            if(guard) QMetaObject::invokeMethod(guard.data(),[guard,id,output=std::move(output),done=std::move(done)]() mutable {
                if(!guard || guard->operation_!=id) return;
                guard->operation_.clear(); guard->cancelled_.reset(); done(std::move(output));
            },Qt::QueuedConnection);
        });
    });
}
void QtContentDownloadStore::cancel(const QString& id) {
    if(id==operation_ && cancelled_) { *cancelled_=true; acquirer_.cancel(id); }
}
}
