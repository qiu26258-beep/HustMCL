#pragma once

#include <QString>
#include <QMetaType>

#include <optional>

namespace hustmcl::application {

enum class PrimaryAction {
    None,
    SignIn,
    Install,
    PrepareEnvironment,
    Launch,
    Retry,
    ContinueInstall,
};

enum class AccountState {
    Offline,
    SignedOut,
    SigningIn,
    SignedIn,
    Expired,
    Error,
};

enum class InstanceState {
    Unchecked,
    Missing,
    Installing,
    Ready,
    Incomplete,
};

enum class JavaState {
    Unchecked,
    Missing,
    Checking,
    Ready,
    Invalid,
};

enum class TaskPhase {
    NotInstalled,
    Resolving,
    PreparingEnvironment,
    Downloading,
    Verifying,
    Ready,
    Committing,
};

enum class TaskState {
    Idle,
    Running,
    Cancelling,
    Cancelled,
    Failed,
    Succeeded,
};

enum class TaskErrorCode {
    None,
    Network,
    Disk,
    Permission,
    Integrity,
    Metadata,
    Launch,
    Unknown,
};

struct AccountSnapshot {
    AccountSnapshot() = default;
    AccountSnapshot(AccountState s, QString name, QString provider)
        : state(s), displayName(std::move(name)), providerLabel(std::move(provider)) {}
    AccountState state = AccountState::SignedOut;
    QString displayName;
    QString providerLabel;
    QString message;
    bool busy = false;
    bool canSignIn = true;
    bool canSignOut = false;
    bool remembered = false;
};

struct InstanceSnapshot {
    QString instanceId;
    QString displayName;
    QString versionLabel;
    QString editionLabel;
    InstanceState state = InstanceState::Missing;
};

struct EnvironmentSnapshot {
    JavaState state = JavaState::Missing;
    QString versionLabel;
    QString architectureLabel;
    QString sourceLabel;
};

struct LaunchSummary {
    int memoryMb = 0;
    bool closeLauncherAfterLaunch = false;
};

struct HomeSnapshot {
    bool offlineMode = false;
    bool canChangeLoginMode = false;
    bool sessionActive = false;
    QString statusTitle;
    QString primaryLabel;
    QString dataPath;
    QString logPath;
    QString greeting;
    QString timeLabel;
    AccountSnapshot account;
    InstanceSnapshot instance;
    EnvironmentSnapshot environment;
    PrimaryAction primaryAction = PrimaryAction::None;
    bool primaryEnabled = false;
    QString primaryReason;
    LaunchSummary launch;
};

struct TaskError {
    TaskErrorCode code = TaskErrorCode::Unknown;
    QString summary;
    QString detail;
    bool retryable = false;
};

struct TaskSnapshot {
    QString taskId;
    QString instanceId;
    TaskPhase phase = TaskPhase::NotInstalled;
    TaskState state = TaskState::Idle;
    qint64 completedBytes = 0;
    std::optional<qint64> totalBytes;
    bool cancelAvailable = false;
    QString cancelReason;
    bool retryAvailable = false;
    QString retryReason;
    std::optional<TaskError> error;
    QString phaseDetail;
};

struct InstallSnapshot {
    TaskSnapshot task;
    QString instanceDisplayName;
    QString versionLabel;
    QString variantLabel;
    bool recommended = false;
    bool verified = false;
    QString targetPath;
    std::optional<qint64> requiredBytes;
    std::optional<qint64> freeBytes;
};

} // namespace hustmcl::application

Q_DECLARE_METATYPE(hustmcl::application::HomeSnapshot)
Q_DECLARE_METATYPE(hustmcl::application::InstallSnapshot)
Q_DECLARE_METATYPE(hustmcl::application::TaskSnapshot)
Q_DECLARE_METATYPE(hustmcl::application::TaskError)
