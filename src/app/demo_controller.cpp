#include "demo_controller.h"

#include <QTimer>

namespace hustmcl::app {

using application::AccountSnapshot;
using application::AccountState;
using application::EnvironmentSnapshot;
using application::HomeSnapshot;
using application::InstallSnapshot;
using application::InstanceState;
using application::InstanceSnapshot;
using application::JavaState;
using application::LaunchSummary;
using application::PrimaryAction;
using application::TaskError;
using application::TaskErrorCode;
using application::TaskPhase;
using application::TaskSnapshot;
using application::TaskState;

namespace {
constexpr qint64 kMegabyte = 1024LL * 1024LL;
constexpr qint64 kGigabyte = 1024LL * 1024LL * 1024LL;
}

DemoController::DemoController(QObject* parent)
    : FrontendGateway(parent)
{
    seedHome();
    setScenario(DemoScenario::HomeReady);
}

HomeSnapshot DemoController::homeSnapshot() const
{
    return m_home;
}

InstallSnapshot DemoController::installSnapshot() const
{
    return m_install;
}

void DemoController::requestPrimaryAction()
{
    // Demo mode intentionally does not fabricate a login, download or launch.
    // MainWindow provides the explicit "not connected" feedback.
}

void DemoController::requestCancel(const QString& taskId)
{
    if (taskId != m_install.task.taskId
        || m_install.task.state != TaskState::Running
        || !m_install.task.cancelAvailable) {
        return;
    }
    m_install.task.state = TaskState::Cancelling;
    m_install.task.cancelAvailable = false;
    m_install.task.cancelReason = QStringLiteral("正在停止任务并清理临时状态。");
    m_install.task.retryAvailable = false;
    m_install.task.phaseDetail = QStringLiteral("正在取消…");
    emit installChanged(m_install);
    startCancellationTimer(taskId);
}

void DemoController::requestRetry(const QString& taskId)
{
    if (taskId != m_install.task.taskId) {
        return;
    }
    if (m_install.task.state != TaskState::Failed
        && m_install.task.state != TaskState::Cancelled) {
        return;
    }
    const QString newId = nextTaskId();
    m_install = baseInstall(newId);
    m_install.task.phase = TaskPhase::Downloading;
    m_install.task.state = TaskState::Running;
    m_install.task.phaseDetail = QStringLiteral("正在检查缓存并继续下载");
    m_install.task.cancelAvailable = true;
    m_install.task.retryAvailable = false;
    emit installChanged(m_install);
}

void DemoController::setScenario(DemoScenario scenario)
{
    m_scenario = scenario;
    seedHome();
    m_install = baseInstall(nextTaskId());
    m_install.task.phase = TaskPhase::Downloading;
    m_install.task.state = TaskState::Running;
    m_install.task.cancelAvailable = true;
    m_install.task.phaseDetail = QStringLiteral("正在下载游戏资源");
    m_install.task.completedBytes = static_cast<qint64>(128.4 * static_cast<double>(kMegabyte));

    switch (scenario) {
    case DemoScenario::HomeReady:
    case DemoScenario::InstallUnknown:
        break;
    case DemoScenario::InstallKnown:
        m_install.task.totalBytes = static_cast<qint64>(1.2 * static_cast<double>(kGigabyte));
        m_install.task.completedBytes = 320LL * kMegabyte;
        break;
    case DemoScenario::InstallCancelling:
        m_install.task.state = TaskState::Cancelling;
        m_install.task.cancelAvailable = false;
        m_install.task.cancelReason = QStringLiteral("正在停止任务并清理临时状态。");
        m_install.task.phaseDetail = QStringLiteral("正在取消…");
        break;
    case DemoScenario::InstallCancelled:
        m_install.task.state = TaskState::Cancelled;
        m_install.task.cancelAvailable = false;
        m_install.task.retryAvailable = true;
        m_install.task.retryReason = QStringLiteral("已校验文件保留，可稍后继续。");
        m_install.task.phaseDetail = QStringLiteral("已取消");
        break;
    case DemoScenario::InstallCommitting:
        m_install.task.phase = TaskPhase::Committing;
        m_install.task.state = TaskState::Running;
        m_install.task.totalBytes = static_cast<qint64>(1.2 * static_cast<double>(kGigabyte));
        m_install.task.completedBytes = m_install.task.totalBytes.value();
        m_install.task.cancelAvailable = false;
        m_install.task.cancelReason = QStringLiteral("正在完成安装，无法取消。");
        m_install.task.phaseDetail = QStringLiteral("正在完成安装");
        break;
    case DemoScenario::InstallFailed:
        m_install.task.state = TaskState::Failed;
        m_install.task.cancelAvailable = false;
        m_install.task.retryAvailable = true;
        m_install.task.retryReason = QStringLiteral("从检查缓存开始重试。");
        m_install.task.phaseDetail = QStringLiteral("下载中断");
        m_install.task.error = TaskError{
            TaskErrorCode::Network,
            QStringLiteral("网络连接中断"),
            QStringLiteral("已校验文件会保留，可以从检查缓存开始重试。"),
            true,
        };
        break;
    case DemoScenario::InstallLongPath:
        m_install.targetPath = QStringLiteral(
            "D:\\HustMCL\\游戏目录\\华中科技大学 校园生存 示例实例\\campus-demo-with-a-very-long-instance-name\\minecraft\\versions\\1.21.4\\saves");
        break;
    }
    emitAll();
}

DemoScenario DemoController::scenario() const
{
    return m_scenario;
}

void DemoController::finishCancellation(const QString& taskId)
{
    if (taskId != m_install.task.taskId || m_install.task.state != TaskState::Cancelling) {
        return;
    }
    m_install.task.state = TaskState::Cancelled;
    m_install.task.cancelAvailable = false;
    m_install.task.retryAvailable = true;
    m_install.task.retryReason = QStringLiteral("已校验文件保留，可稍后继续。");
    m_install.task.phaseDetail = QStringLiteral("已取消");
    emit installChanged(m_install);
}

std::optional<DemoScenario> DemoController::scenarioFromName(const QString& name)
{
    if (name == QStringLiteral("home-ready")) return DemoScenario::HomeReady;
    if (name == QStringLiteral("install-unknown")) return DemoScenario::InstallUnknown;
    if (name == QStringLiteral("install-known")) return DemoScenario::InstallKnown;
    if (name == QStringLiteral("install-cancelling")) return DemoScenario::InstallCancelling;
    if (name == QStringLiteral("install-cancelled")) return DemoScenario::InstallCancelled;
    if (name == QStringLiteral("install-committing")) return DemoScenario::InstallCommitting;
    if (name == QStringLiteral("install-failed")) return DemoScenario::InstallFailed;
    if (name == QStringLiteral("install-long-path")) return DemoScenario::InstallLongPath;
    return std::nullopt;
}

QString DemoController::scenarioName(DemoScenario scenario)
{
    switch (scenario) {
    case DemoScenario::HomeReady: return QStringLiteral("home-ready");
    case DemoScenario::InstallUnknown: return QStringLiteral("install-unknown");
    case DemoScenario::InstallKnown: return QStringLiteral("install-known");
    case DemoScenario::InstallCancelling: return QStringLiteral("install-cancelling");
    case DemoScenario::InstallCancelled: return QStringLiteral("install-cancelled");
    case DemoScenario::InstallCommitting: return QStringLiteral("install-committing");
    case DemoScenario::InstallFailed: return QStringLiteral("install-failed");
    case DemoScenario::InstallLongPath: return QStringLiteral("install-long-path");
    }
    return QStringLiteral("home-ready");
}

QString DemoController::nextTaskId()
{
    ++m_taskSerial;
    return QStringLiteral("task-demo-%1").arg(m_taskSerial, 3, 10, QLatin1Char('0'));
}

void DemoController::seedHome()
{
    m_home = HomeSnapshot{};
    m_home.greeting = QStringLiteral("下午好，林知远");
    m_home.timeLabel = QStringLiteral("周五 · 16:42");
    m_home.account = AccountSnapshot{AccountState::SignedIn, QStringLiteral("林知远"), QStringLiteral("微软账号")};
    m_home.instance = InstanceSnapshot{
        QStringLiteral("campus-demo"),
        QStringLiteral("校园生存"),
        QStringLiteral("1.21.4"),
        QStringLiteral("原版生存"),
        InstanceState::Ready,
    };
    m_home.environment = EnvironmentSnapshot{
        JavaState::Ready,
        QStringLiteral("由应用选择"),
        QStringLiteral("x64"),
        QStringLiteral("自动"),
    };
    m_home.primaryAction = PrimaryAction::Launch;
    m_home.primaryEnabled = true;
    m_home.primaryReason = QStringLiteral("账号、Java 与实例均已就绪");
    m_home.launch = LaunchSummary{4096, true};
}

InstallSnapshot DemoController::baseInstall(const QString& id) const
{
    InstallSnapshot snapshot;
    snapshot.instanceDisplayName = QStringLiteral("校园生存");
    snapshot.versionLabel = QStringLiteral("1.21.4");
    snapshot.variantLabel = QStringLiteral("正式版 · 原版生存");
    snapshot.recommended = true;
    snapshot.verified = true;
    snapshot.targetPath = QStringLiteral("D:\\HustMCL\\instances\\campus-demo");
    snapshot.requiredBytes = static_cast<qint64>(1.2 * static_cast<double>(kGigabyte));
    snapshot.freeBytes = static_cast<qint64>(86.5 * static_cast<double>(kGigabyte));
    snapshot.task.taskId = id;
    snapshot.task.instanceId = QStringLiteral("campus-demo");
    snapshot.task.phase = TaskPhase::NotInstalled;
    snapshot.task.state = TaskState::Idle;
    snapshot.task.completedBytes = 0;
    return snapshot;
}

void DemoController::emitAll()
{
    emit homeChanged(m_home);
    emit installChanged(m_install);
}

void DemoController::startCancellationTimer(const QString& taskId)
{
    QTimer::singleShot(650, this, [this, taskId]() {
        finishCancellation(taskId);
    });
}

} // namespace hustmcl::app
