#include <hustmcl/application/offline_controller.h>
#include <hustmcl/application/authenticated_launch.h>
#include <QDate>
#include <QPointer>

namespace hustmcl::application {
OfflineController::OfflineController(LaunchPreparation& preparation, GameProcess& process,
                                     QString dataRoot, QString versionId, QObject* parent)
    : FrontendGateway(parent), preparation_(preparation), process_(process)
{
    home_.offlineMode = true;
    home_.greeting = QStringLiteral("欢迎回到 HustMCL");
    home_.timeLabel = QDate::currentDate().toString(QStringLiteral("yyyy.MM.dd"));
    home_.account = {AccountState::Offline, QStringLiteral("HustPlayer"), QStringLiteral("本地离线身份")};
    home_.instance = {versionId, QStringLiteral("我的原版世界"), versionId,
                      QStringLiteral("Java 原版 · 离线单人"), InstanceState::Unchecked};
    home_.environment = {JavaState::Unchecked, QStringLiteral("启动前检测"), QStringLiteral("x64"), QStringLiteral("随包运行时")};
    home_.launch.memoryMb = 2048;
    home_.dataPath = std::move(dataRoot);
    home_.statusTitle = QStringLiteral("从这里开启世界");
    home_.primaryAction = PrimaryAction::Launch;
    home_.primaryEnabled = true;
    home_.primaryLabel = QStringLiteral("启动游戏");
    home_.primaryReason = QStringLiteral("启动前将校验本地文件并检测 Java；使用独立的离线身份。");
    install_.instanceDisplayName = home_.instance.displayName;
    install_.versionLabel = versionId;
    install_.variantLabel = QStringLiteral("已随包提供 · 本地原版");
    install_.targetPath = home_.dataPath;
    install_.task.instanceId = versionId;
    install_.task.phaseDetail = home_.primaryReason;
    if (versionId.isEmpty()) {
        home_.instance.displayName = QStringLiteral("尚未安装游戏");
        home_.primaryEnabled = false;
        home_.primaryLabel = QStringLiteral("请先导入整合包");
        home_.statusTitle = QStringLiteral("导入整合包以开始");
        home_.primaryReason = QStringLiteral("将支持的 ZIP 或 MRPACK 拖入主页，按需下载游戏与运行环境。");
        install_.variantLabel = QStringLiteral("尚未安装");
        install_.instanceDisplayName = home_.instance.displayName;
        install_.task.phaseDetail = home_.primaryReason;
    }
    connect(&preparation_, &LaunchPreparation::changed, this, &OfflineController::preparationChanged);
    connect(&preparation_, &LaunchPreparation::prepared, this, &OfflineController::prepared);
}
void OfflineController::publish()
{
    home_.canChangeLoginMode = account_ && !home_.sessionActive && !account_->snapshot().busy;
    const QPointer<OfflineController> guard(this);
    const auto home = home_;
    const auto install = install_;
    emit homeChanged(home);
    if (guard) emit installChanged(install);
}
void OfflineController::setAccountController(AccountController* account)
{
    if (home_.sessionActive || account_ == account) return;
    if (account_) disconnect(account_, nullptr, this, nullptr);
    account_ = account;
    home_.offlineMode = !account_ || offlineSelected_;
    if (account_) connect(account_, &AccountController::changed, this, [this] { accountChanged(); });
    accountChanged();
}
bool OfflineController::setOfflineMode(bool offline)
{
    if (!account_ || home_.sessionActive || account_->snapshot().busy) return false;
    if (home_.offlineMode == offline) return true;
    if (!persist({offlinePlayerName_, home_.launch.memoryMb, offline})) return false;
    offlineSelected_ = offline;
    home_.offlineMode = offline;
    const QPointer<OfflineController> guard(this);
    accountChanged();
    // Restoring a saved account never opens the browser or starts a game.
    if (guard && !offline) account_->restore();
    return true;
}
void OfflineController::accountChanged()
{
    if (home_.offlineMode) {
        home_.account = {AccountState::Offline, offlinePlayerName_, QStringLiteral("本地离线身份")};
        home_.account.canSignIn = false;
        if (!home_.sessionActive) {
            const bool installed = !home_.instance.instanceId.isEmpty();
            home_.instance.editionLabel = QStringLiteral("Java · 离线单人");
            install_.variantLabel = QStringLiteral("Minecraft Java · 离线");
            home_.primaryAction = PrimaryAction::Launch;
            home_.primaryEnabled = !importActive_ && installed;
            home_.primaryLabel = importActive_ ? QStringLiteral("正在安装…") : installed ? QStringLiteral("启动游戏") : QStringLiteral("请先安装游戏");
            home_.statusTitle = QStringLiteral("离线玩家已就绪");
            home_.primaryReason = installed ? QStringLiteral("将使用离线玩家启动本地游戏；不会进行微软登录。")
                                           : QStringLiteral("请在首页安装原版或导入整合包，再手动启动。");
        }
        publish();
        return;
    }
    if (!account_) return;
    home_.account = account_->snapshot();
    if (home_.sessionActive) { publish(); return; }
    home_.instance.editionLabel = QStringLiteral("Java · 正版账号");
    install_.variantLabel = QStringLiteral("Minecraft Java");
    const bool ready = account_->session() != nullptr;
    home_.primaryAction = ready ? PrimaryAction::Launch : PrimaryAction::SignIn;
    home_.primaryLabel = home_.account.busy ? QStringLiteral("登录中…") : ready ? QStringLiteral("启动游戏") : QStringLiteral("登录微软账号");
    home_.primaryEnabled = !importActive_ && !home_.account.busy && (!ready || !home_.instance.instanceId.isEmpty());
    home_.statusTitle = ready ? QStringLiteral("正版账号已就绪") : QStringLiteral("登录 Minecraft");
    home_.primaryReason = home_.account.message;
    if (ready && home_.instance.instanceId.isEmpty()) {
        home_.primaryLabel = QStringLiteral("请先安装游戏");
        home_.primaryReason = QStringLiteral("账号已验证，请安装原版或导入支持的整合包，再手动启动。");
    }
    publish();
}
void OfflineController::requestSignIn() { if (account_ && !home_.offlineMode && !home_.sessionActive) account_->signIn(); }
void OfflineController::requestSignOut() { if (account_ && !home_.offlineMode && !home_.sessionActive) account_->signOut(); }
void OfflineController::requestCancelSignIn() { if (account_ && !home_.offlineMode) account_->cancel(); }
bool OfflineController::setPlayerName(const QString& name)
{
    if (!home_.offlineMode || home_.sessionActive || std::holds_alternative<OfflineLaunchError>(makeOfflineLaunch({}, name))) return false;
    if (!persist({name, home_.launch.memoryMb, offlineSelected_})) return false;
    offlinePlayerName_ = name;
    home_.account.displayName = name;
    publish();
    return true;
}
bool OfflineController::setMemoryMb(int memoryMb)
{
    if (home_.sessionActive || !validMemoryMb(memoryMb)) return false;
    if (!persist({offlinePlayerName_, memoryMb, offlineSelected_})) return false;
    home_.launch.memoryMb = memoryMb;
    preparation_.setMemoryMb(memoryMb);
    publish();
    return true;
}
void OfflineController::applySettings(const LauncherSettings& settings)
{
    if (home_.sessionActive) return;
    if (!std::holds_alternative<OfflineLaunchError>(makeOfflineLaunch({}, settings.playerName))) {
        offlinePlayerName_ = settings.playerName;
        if (home_.offlineMode) home_.account.displayName = settings.playerName;
    }
    offlineSelected_ = settings.offlineMode;
    if (account_) home_.offlineMode = offlineSelected_;
    if (validMemoryMb(settings.memoryMb)) {
        home_.launch.memoryMb = settings.memoryMb;
        preparation_.setMemoryMb(settings.memoryMb);
    }
    if (account_) accountChanged(); else publish();
}
bool OfflineController::persist(const LauncherSettings& settings)
{
    return !store_ || store_->save(settings);
}
bool OfflineController::selectInstance(const InstalledInstance& instance)
{
    if (home_.sessionActive || importActive_ || instance.id.isEmpty() || instance.root.isEmpty()) return false;
    home_.instance = {instance.id, instance.name, instance.versionLabel,
        QStringLiteral("Java · 离线单人"), InstanceState::Unchecked};
    home_.dataPath = instance.root;
    home_.logPath.clear();
    home_.environment = {JavaState::Unchecked, QStringLiteral("启动前检测"), QStringLiteral("x64"), QStringLiteral("实例运行时")};
    home_.primaryAction = PrimaryAction::Launch;
    home_.primaryEnabled = true;
    home_.primaryLabel = QStringLiteral("启动游戏");
    home_.statusTitle = QStringLiteral("已选择实例");
    home_.primaryReason = QStringLiteral("点击启动前将重新校验游戏、模组和 Java。");
    install_ = {};
    install_.instanceDisplayName = instance.name;
    install_.versionLabel = instance.versionLabel;
    install_.variantLabel = QStringLiteral("本地实例");
    install_.targetPath = instance.root;
    install_.task.instanceId = instance.id;
    install_.task.phaseDetail = home_.primaryReason;
    if (account_) { accountChanged(); return true; }
    publish();
    return true;
}
void OfflineController::setImportActive(bool active)
{
    if (importActive_ == active) return;
    importActive_ = active;
    if (home_.sessionActive) return;
    home_.primaryEnabled = !active && !home_.instance.instanceId.isEmpty();
    home_.primaryLabel = active ? QStringLiteral("正在安装整合包…") : home_.instance.instanceId.isEmpty() ? QStringLiteral("请先导入整合包") : QStringLiteral("启动游戏");
    home_.primaryReason = active ? QStringLiteral("安装完成后可手动启动游戏。")
                                : QStringLiteral("启动前将重新校验当前实例和 Java。");
    if (account_) { accountChanged(); return; }
    publish();
}
void OfflineController::requestPrimaryAction()
{
    if (account_ && !home_.offlineMode && !home_.sessionActive && !importActive_ && !account_->session()) {
        if (home_.account.state == AccountState::SignedIn && account_->refresh()) return;
        requestSignIn(); return;
    }
    if (home_.sessionActive || importActive_ || home_.instance.instanceId.isEmpty()) return;
    launching_ = false;
    home_.sessionActive = true;
    if (account_) account_->setLaunchActive(true);
    home_.primaryEnabled = false;
    install_.verified = false;
    install_.task.error.reset();
    install_.task.retryAvailable = false;
    static_cast<void>(preparation_.start(home_.instance.instanceId));
}
void OfflineController::requestCancel(const QString& taskId)
{
    if (!launching_) static_cast<void>(preparation_.cancel(taskId));
}
void OfflineController::requestRetry(const QString& taskId)
{
    if (taskId == install_.task.taskId && install_.task.retryAvailable) requestPrimaryAction();
}
void OfflineController::preparationChanged(const PreparationSnapshot& snapshot)
{
    if (launching_) return;
    auto& task = install_.task;
    task.taskId = snapshot.taskId;
    task.instanceId = snapshot.instanceId;
    task.cancelAvailable = snapshot.cancelAvailable;
    task.state = TaskState::Running;
    task.retryAvailable = false;
    home_.primaryEnabled = false;
    switch (snapshot.phase) {
    case PreparationPhase::LoadingInput:
        task.phase = TaskPhase::Verifying;
        task.phaseDetail = QStringLiteral("正在校验本地原版文件，请稍候。校验不会修改游戏文件或存档。");
        home_.statusTitle = QStringLiteral("检查游戏文件");
        home_.primaryLabel = QStringLiteral("校验中…");
        home_.environment.state = JavaState::Unchecked;
        home_.instance.state = InstanceState::Unchecked;
        break;
    case PreparationPhase::ProbingJava:
        home_.environment.state = JavaState::Checking;
        task.phase = TaskPhase::PreparingEnvironment;
        task.phaseDetail = QStringLiteral("文件校验通过，正在检测随包 Java 的版本与架构。");
        home_.statusTitle = QStringLiteral("检测 Java");
        home_.primaryLabel = QStringLiteral("检测中…");
        install_.verified = true;
        home_.instance.state = InstanceState::Ready;
        break;
    case PreparationPhase::Planning:
    case PreparationPhase::Prepared:
        task.phase = TaskPhase::Ready;
        task.phaseDetail = home_.offlineMode ? QStringLiteral("正在准备离线启动参数。") : QStringLiteral("正在准备正版启动参数。");
        home_.primaryLabel = QStringLiteral("准备启动…");
        break;
    case PreparationPhase::Cancelling:
        task.state = TaskState::Cancelling;
        task.phaseDetail = QStringLiteral("正在等待 Java 检测结束后取消。");
        home_.primaryLabel = QStringLiteral("取消中…");
        break;
    case PreparationPhase::Cancelled:
        task.state = TaskState::Cancelled;
        task.phaseDetail = QStringLiteral("已取消启动，未创建游戏进程。");
        home_.sessionActive = false;
        if (account_) account_->setLaunchActive(false);
        home_.primaryEnabled = true;
        home_.primaryAction = PrimaryAction::Launch;
        home_.primaryLabel = QStringLiteral("启动游戏");
        home_.statusTitle = QStringLiteral("已取消");
        home_.environment.state = JavaState::Unchecked;
        break;
    case PreparationPhase::Failed: {
        const bool javaFailure = snapshot.error &&
            (snapshot.error->code == PreparationErrorCode::JavaUnavailable || snapshot.error->code == PreparationErrorCode::InvalidManualJava);
        if (javaFailure) home_.environment.state = JavaState::Invalid;
        else home_.instance.state = InstanceState::Incomplete;
        fail(javaFailure ? TaskErrorCode::Launch : TaskErrorCode::Integrity,
             javaFailure ? QStringLiteral("Java 检测未通过") : QStringLiteral("本地启动准备未通过"),
             javaFailure ? QStringLiteral("请检查 runtime 目录中的 Java 是否完整且符合游戏要求。")
                         : QStringLiteral("请确认版本、assets 和 libraries 目录完整。重新解压体验包后重试；已有存档不会被删除。"));
        return;
    }
    case PreparationPhase::Idle: return;
    }
    home_.primaryReason = task.phaseDetail;
    publish();
}
void OfflineController::prepared(const QString& taskId, const core::LaunchPlan& plan)
{
    if (!home_.sessionActive || launching_ || taskId != install_.task.taskId) return;
    GameLaunch identity;
    if (!home_.offlineMode && account_) {
        const auto* session = account_->session();
        if (!session) {
            fail(TaskErrorCode::Permission, QStringLiteral("登录已过期"), QStringLiteral("请重新登录微软账号后启动。")); return;
        }
        const auto result = makeAuthenticatedLaunch(plan, *session);
        if (const auto* value = std::get_if<GameLaunch>(&result)) identity = *value;
        else { fail(TaskErrorCode::Permission, QStringLiteral("账号验证未通过"), QStringLiteral("请重新登录后重试。")); return; }
    } else {
        const auto resolved = makeOfflineLaunch(plan, home_.account.displayName);
        if (!std::holds_alternative<OfflineLaunch>(resolved)) {
            fail(TaskErrorCode::Launch, QStringLiteral("离线身份无效"), QStringLiteral("玩家名须为 1–16 位英文字母、数字或下划线。"));
            return;
        }
        identity = std::get<OfflineLaunch>(resolved);
    }
    launching_ = true;
    home_.environment.state = JavaState::Ready;
    home_.environment.versionLabel = QStringLiteral("Java %1").arg(plan.runtime.majorVersion);
    home_.statusTitle = QStringLiteral("正在启动游戏");
    home_.primaryLabel = QStringLiteral("启动中…");
    home_.primaryReason = QStringLiteral("正在创建 Minecraft 进程。");
    install_.task.cancelAvailable = false;
    install_.task.phaseDetail = home_.primaryReason;
    const QPointer<OfflineController> guard(this);
    publish();
    if (!guard || !home_.sessionActive) return;
    process_.start(taskId, plan, identity, [guard, taskId](GameProcessEvent event) {
        if (guard) guard->processEvent(taskId, std::move(event));
    });
}
void OfflineController::processEvent(const QString& taskId, GameProcessEvent event)
{
    if (!launching_ || taskId != install_.task.taskId) return;
    if (!event.logPath.isEmpty()) home_.logPath = event.logPath;
    if (event.kind == GameProcessEvent::Kind::Failed) {
        const bool busy = event.error == GameProcessError::Busy;
        fail(TaskErrorCode::Launch, busy ? QStringLiteral("这个实例正在使用中") : QStringLiteral("游戏启动或运行失败"),
             busy ? QStringLiteral("请先退出此实例的另一个游戏窗口，再重试。")
                  : QStringLiteral("错误码 %1；退出码 %2。请在日志页查看本次运行日志。")
                        .arg(static_cast<int>(event.error)).arg(event.exitCode));
        return;
    }
    if (event.kind == GameProcessEvent::Kind::Started) {
        home_.statusTitle = QStringLiteral("Minecraft 正在运行");
        home_.primaryLabel = QStringLiteral("游戏运行中");
        home_.primaryReason = QStringLiteral("游戏窗口正在打开。退出游戏后可以再次启动；关闭启动器窗口不会结束游戏。");
    } else {
        launching_ = false;
        home_.sessionActive = false;
        if (account_) account_->setLaunchActive(false);
        home_.primaryEnabled = true;
        home_.primaryAction = PrimaryAction::Launch;
        home_.primaryLabel = QStringLiteral("再次启动");
        home_.statusTitle = QStringLiteral("游戏已正常退出");
        home_.primaryReason = QStringLiteral("本次游戏退出码为 0，存档保存在游戏目录中。");
        install_.task.state = TaskState::Succeeded;
    }
    install_.task.phaseDetail = home_.primaryReason;
    publish();
}
void OfflineController::fail(TaskErrorCode code, const QString& summary, const QString& detail)
{
    launching_ = false;
    home_.sessionActive = false;
    if (account_) account_->setLaunchActive(false);
    home_.primaryEnabled = true;
    home_.primaryAction = PrimaryAction::Retry;
    home_.primaryLabel = QStringLiteral("重试启动");
    home_.statusTitle = summary;
    home_.primaryReason = detail;
    install_.task.state = TaskState::Failed;
    install_.task.cancelAvailable = false;
    install_.task.retryAvailable = true;
    install_.task.phaseDetail = summary;
    install_.task.error = TaskError{code, summary, detail, true};
    publish();
}
}
