#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QObject>

namespace hustmcl::application {

// Minimal page-facing contract. The UI reads snapshots and emits intent; it
// never infers account, Java, install or launch state.
class FrontendGateway : public QObject {
    Q_OBJECT

public:
    explicit FrontendGateway(QObject* parent = nullptr);
    ~FrontendGateway() override;

    virtual HomeSnapshot homeSnapshot() const = 0;
    virtual InstallSnapshot installSnapshot() const = 0;

    virtual void requestPrimaryAction() = 0;
    virtual void requestCancel(const QString& taskId) = 0;
    virtual void requestRetry(const QString& taskId) = 0;

signals:
    void homeChanged(const hustmcl::application::HomeSnapshot& snapshot);
    void installChanged(const hustmcl::application::InstallSnapshot& snapshot);
};

} // namespace hustmcl::application
