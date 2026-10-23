#pragma once
#include <hustmcl/application/file_acquisition.h>
#include <QObject>
#include <atomic>
#include <memory>
class QThread;
namespace hustmcl::services {
class QtFileAcquirer final : public QObject, public application::FileAcquirer {
public:
    struct Options { int timeoutMs = 30000; bool allowLoopbackHttp = false; };
    explicit QtFileAcquirer(Options options, QObject* parent = nullptr);
    ~QtFileAcquirer() override;
    void acquire(const QString&, const QString&, const core::DownloadDescriptor&, Completion) override;
    void cancel(const QString&) override;
    application::FileError beginSession(const QString&, int) override;
    void endSession() override;
private:
    struct Session;
    std::unique_ptr<Session> session_;
    Options options_;
    QString taskId_;
    QThread* worker_ = nullptr;
    std::shared_ptr<std::atomic_bool> cancelled_;
};
}
