#pragma once

#include <hustmcl/application/launch_preparation.h>

#include <QByteArray>
#include <QObject>
#include <QThreadPool>

namespace hustmcl::services {

// Read-only developer input source; not an installed-instance store.
class MetadataFileSource final : public QObject, public application::LaunchInputSource {
public:
    MetadataFileSource(QString instanceId, QString file, QByteArray expectedSha1,
                       application::LaunchInput settings, QObject* parent = nullptr);
    ~MetadataFileSource() override;
    void load(const QString& instanceId, Completion complete) override;
private:
    QString instanceId_;
    QString file_;
    QByteArray expectedSha1_;
    application::LaunchInput settings_;
    QThreadPool workers_;
    bool busy_ = false;
};

} // namespace hustmcl::services
