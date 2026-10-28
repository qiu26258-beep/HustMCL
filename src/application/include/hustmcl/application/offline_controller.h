#pragma once
#include <hustmcl/application/frontend_gateway.h>
#include <hustmcl/application/launch_preparation.h>
#include <hustmcl/application/offline_launch.h>
#include <hustmcl/application/modpack_import.h>
#include <hustmcl/application/launcher_settings.h>
#include <hustmcl/application/account.h>

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

// Owns launch state; online identity is supplied by the optional account use case.
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
    void requestSignIn() override;
    void requestSignOut() override;
    void requestCancelSignIn() override;
    void setAccountController(AccountController* account);
    bool setPlayerName(const QString& name) override;
    bool setMemoryMb(int memoryMb) override;
    // Applies stored preferences without writing them back; invalid fields are rejected individually.
    void applySettings(const LauncherSettings& settings);
    // Later accepted changes are saved through the store. The store must outlive the controller.
    void setSettingsStore(SettingsStore* store) { store_ = store; }
    bool selectInstance(const InstalledInstance& instance);
    void setImportActive(bool active);
private:
    void publish();
    void accountChanged();
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
    SettingsStore* store_ = nullptr;
    AccountController* account_ = nullptr;
    QString offlinePlayerName_;
    void persist();
};
}
