#pragma once
#include <hustmcl/application/content_market.h>
#include <hustmcl/application/file_acquisition.h>
#include <QThreadPool>
#include <atomic>
#include <memory>

namespace hustmcl::services {
class QtContentDownloadStore final : public QObject, public application::ContentDownloadStore {
public:
    QtContentDownloadStore(QString dataRoot, application::FileAcquirer&, QObject* parent = nullptr);
    ~QtContentDownloadStore() override;
    void acquire(const QString&, const application::ContentDownloadRequest&,
                 std::function<void(application::ContentResult<application::ContentDownloadResult>)>) override;
    void cancel(const QString&) override;
private:
    QString root_, operation_;
    application::FileAcquirer& acquirer_;
    QThreadPool workers_;
    std::shared_ptr<std::atomic_bool> cancelled_;
};
}
