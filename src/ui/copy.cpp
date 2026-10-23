#include "copy.h"

#include <QLocale>

namespace hustmcl::ui::copy {

using application::AccountState;
using application::InstanceState;
using application::JavaState;
using application::PrimaryAction;
using application::TaskErrorCode;
using application::TaskPhase;
using application::TaskState;

QString primaryActionText(PrimaryAction action)
{
    switch (action) {
    case PrimaryAction::SignIn:
        return QStringLiteral("登录");
    case PrimaryAction::Install:
        return QStringLiteral("安装游戏");
    case PrimaryAction::PrepareEnvironment:
        return QStringLiteral("准备环境");
    case PrimaryAction::Launch:
        return QStringLiteral("启动游戏");
    case PrimaryAction::Retry:
        return QStringLiteral("重试");
    case PrimaryAction::ContinueInstall:
        return QStringLiteral("继续安装");
    case PrimaryAction::None:
        break;
    }
    return {};
}

QString accountStateText(AccountState state)
{
    switch (state) {
    case AccountState::Offline:
        return QStringLiteral("本地离线");
    case AccountState::SignedOut:
        return QStringLiteral("未登录");
    case AccountState::SigningIn:
        return QStringLiteral("登录中");
    case AccountState::SignedIn:
        return QStringLiteral("已连接");
    case AccountState::Expired:
        return QStringLiteral("已过期");
    case AccountState::Error:
        return QStringLiteral("登录异常");
    }
    return QStringLiteral("状态未知");
}

QString instanceStateText(InstanceState state)
{
    switch (state) {
    case InstanceState::Unchecked:
        return QStringLiteral("待检查");
    case InstanceState::Missing:
        return QStringLiteral("未安装");
    case InstanceState::Installing:
        return QStringLiteral("安装中");
    case InstanceState::Ready:
        return QStringLiteral("已就绪");
    case InstanceState::Incomplete:
        return QStringLiteral("不完整");
    }
    return QStringLiteral("状态未知");
}

QString javaStateText(JavaState state)
{
    switch (state) {
    case JavaState::Unchecked:
        return QStringLiteral("待检测");
    case JavaState::Missing:
        return QStringLiteral("未就绪");
    case JavaState::Checking:
        return QStringLiteral("检测中");
    case JavaState::Ready:
        return QStringLiteral("已就绪");
    case JavaState::Invalid:
        return QStringLiteral("无效");
    }
    return QStringLiteral("状态未知");
}

QString taskStateText(TaskState state)
{
    switch (state) {
    case TaskState::Idle:
        return QStringLiteral("尚未开始");
    case TaskState::Running:
        return QStringLiteral("进行中");
    case TaskState::Cancelling:
        return QStringLiteral("正在取消");
    case TaskState::Cancelled:
        return QStringLiteral("已取消");
    case TaskState::Failed:
        return QStringLiteral("失败");
    case TaskState::Succeeded:
        return QStringLiteral("已完成");
    }
    return QStringLiteral("状态未知");
}

QString checkPhaseText(TaskPhase phase)
{
    switch (phase) {
    case TaskPhase::NotInstalled:
        return QStringLiteral("尚未检查");
    case TaskPhase::Resolving:
        return QStringLiteral("读取版本信息");
    case TaskPhase::PreparingEnvironment:
        return QStringLiteral("检测 Java 运行时");
    case TaskPhase::Downloading:
        return QStringLiteral("检查本地文件");
    case TaskPhase::Verifying:
        return QStringLiteral("校验游戏文件");
    case TaskPhase::Ready:
        return QStringLiteral("检查完成");
    case TaskPhase::Committing:
        return QStringLiteral("创建游戏进程");
    }
    return QStringLiteral("状态未知");
}

QString errorCodeText(TaskErrorCode code)
{
    switch (code) {
    case TaskErrorCode::None:
        return QStringLiteral("NONE");
    case TaskErrorCode::Network:
        return QStringLiteral("NETWORK");
    case TaskErrorCode::Disk:
        return QStringLiteral("DISK");
    case TaskErrorCode::Permission:
        return QStringLiteral("PERMISSION");
    case TaskErrorCode::Integrity:
        return QStringLiteral("INTEGRITY");
    case TaskErrorCode::Metadata:
        return QStringLiteral("METADATA");
    case TaskErrorCode::Launch:
        return QStringLiteral("LAUNCH");
    case TaskErrorCode::Unknown:
        break;
    }
    return QStringLiteral("UNKNOWN");
}

QString formatBytes(qint64 bytes)
{
    if (bytes < 0) {
        return QStringLiteral("未知");
    }
    const double value = static_cast<double>(bytes);
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024LL * 1024LL) {
        return QStringLiteral("%1 KB").arg(value / 1024.0, 0, 'f', 1);
    }
    if (bytes < 1024LL * 1024LL * 1024LL) {
        return QStringLiteral("%1 MB").arg(value / (1024.0 * 1024.0), 0, 'f', 1);
    }
    return QStringLiteral("%1 GB").arg(value / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

QString formatMemory(int memoryMb)
{
    return QStringLiteral("%1 MB").arg(memoryMb);
}

double progressFraction(qint64 completedBytes, qint64 totalBytes)
{
    if (completedBytes <= 0 || totalBytes <= 0) {
        return 0.0;
    }
    const double fraction = static_cast<double>(completedBytes) / static_cast<double>(totalBytes);
    return qBound(0.0, fraction, 1.0);
}

QString formatPercent(qint64 completedBytes, qint64 totalBytes)
{
    if (totalBytes <= 0) {
        return {};
    }
    const int percent = static_cast<int>(qRound(progressFraction(completedBytes, totalBytes) * 100.0));
    return QStringLiteral("%1%").arg(percent);
}

} // namespace hustmcl::ui::copy
