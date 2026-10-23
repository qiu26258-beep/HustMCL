#pragma once
#include <hustmcl/application/frontend_gateway.h>
#include <hustmcl/application/launch_preparation.h>
#include <hustmcl/application/offline_launch.h>
#include <hustmcl/application/modpack_import.h>

namespace hustmcl::application {
enum class GameProcessError { Busy, UnsafeDirectory, FailedToStart, AbnormalExit, ReportFailed };
struct GameProcessEvent {
    enum class Kind { Started, Finished, Failed };
    Kind kind;
    QString logPath;
    int exitCode = 0;
    GameProcessError error = GameProcessError::FailedToStart;
};
class GameProcess {
public:
    using Observer = std::function<void(GameProcessEvent)>;
    virtual ~GameProcess() = default;
    // Never invokes a shell. Events arrive on the owning event thread.
    virtual void start(const QString& taskId, const core::LaunchPlan& plan,
                       const OfflineLaunch& identity, Observer observer) = 0;
};

// Owns the real offline UI state. External IO stays behind preparation/process ports.
class OfflineController final : public FrontendGateway {
    Q_OBJECT
public:
    OfflineController(LaunchPreparation& preparation, GameProcess& process,
                      QString dataRoot, QString versionId, QObject* parent = nullptr);
    HomeSnapshot homeSnapshot() const override { return home_; }
    InstallSnapshot installSnapshot() const override { return install_; }
    void requestPrimaryAction() override;
    void requestCancel(const QString& taskId) override;
    void requestRetry(const QString& taskId) override;
    bool setPlayerName(const QString& name) override;
    bool selectInstance(const InstalledInstance& instance);
    void setImportActive(bool active);
private:
    void publish();
    void preparationChanged(const PreparationSnapshot& snapshot);
    void prepared(const QString& taskId, const core::LaunchPlan& plan);
    void processEvent(const QString& taskId, GameProcessEvent event);
    void fail(TaskErrorCode code, const QString& summary, const QString& detail);
    LaunchPreparation& preparation_;
    GameProcess& process_;
    HomeSnapshot home_;
    InstallSnapshot install_;
    bool launching_ = false;
    bool importActive_ = false;
};
}
