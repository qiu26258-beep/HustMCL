#pragma once

#include "hustmcl/application/frontend_gateway.h"
#include "hustmcl/application/frontend_snapshot.h"

#include <QString>

#include <optional>

namespace hustmcl::app {

enum class DemoScenario {
    HomeReady,
    InstallUnknown,
    InstallKnown,
    InstallCancelling,
    InstallCancelled,
    InstallCommitting,
    InstallFailed,
    InstallLongPath,
};

// Explicit demo-only state owner. It is not a production application service;
// all transitions live here so QWidget pages never mutate task state.
class DemoController : public application::FrontendGateway {
public:
    explicit DemoController(QObject* parent = nullptr);

    application::HomeSnapshot homeSnapshot() const override;
    application::InstallSnapshot installSnapshot() const override;

    void requestPrimaryAction() override;
    void requestCancel(const QString& taskId) override;
    void requestRetry(const QString& taskId) override;

    void setScenario(DemoScenario scenario);
    DemoScenario scenario() const;

    // Deterministic completion hook for tests and screenshot fixtures.
    void finishCancellation(const QString& taskId);

    static std::optional<DemoScenario> scenarioFromName(const QString& name);
    static QString scenarioName(DemoScenario scenario);

private:
    QString nextTaskId();
    void seedHome();
    application::InstallSnapshot baseInstall(const QString& id) const;
    void emitAll();
    void startCancellationTimer(const QString& taskId);

    application::HomeSnapshot m_home;
    application::InstallSnapshot m_install;
    DemoScenario m_scenario = DemoScenario::HomeReady;
    int m_taskSerial = 0;
};

} // namespace hustmcl::app
