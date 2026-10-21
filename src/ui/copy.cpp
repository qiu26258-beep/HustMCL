#include "copy.h"

#include <QLocale>

namespace hustmcl::ui::copy {

using application::AccountState;
using application::InstanceState;
using application::JavaState;
using application::PrimaryAction;
using application::TaskErrorCode;
using application::TaskPhase;

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

QString phaseTitle(TaskPhase phase)
{
    switch (phase) {
    case TaskPhase::NotInstalled:
        return QStringLiteral("未安装");
    case TaskPhase::Resolving:
        return QStringLiteral("解析版本");
    case TaskPhase::PreparingEnvironment:
        return QStringLiteral("准备环境");
    case TaskPhase::Downloading:
        return QStringLiteral("下载文件");
    case TaskPhase::Verifying:
        return QStringLiteral("校验文件");
    case TaskPhase::Ready:
    case TaskPhase::Committing:
        return QStringLiteral("就绪");
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

QString formatPercent(qint64 completedBytes, qint64 totalBytes)
{
    if (totalBytes <= 0) {
        return {};
    }
    const int percent = qBound(0, static_cast<int>((static_cast<double>(completedBytes) / static_cast<double>(totalBytes)) * 100.0 + 0.5), 100);
    return QStringLiteral("%1%").arg(percent);
}

} // namespace hustmcl::ui::copy
