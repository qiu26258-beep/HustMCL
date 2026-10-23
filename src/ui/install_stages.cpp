#include "install_stages.h"

namespace hustmcl::ui {

const std::array<InstallStageDefinition, 6>& installStageDefinitions()
{
    static const std::array<InstallStageDefinition, 6> stages = {{
        {QStringLiteral("未安装"), QStringLiteral("目标目录为空")},
        {QStringLiteral("解析版本"), QStringLiteral("文件清单已确认")},
        {QStringLiteral("准备环境"), QStringLiteral("Java 运行时已就绪")},
        {QStringLiteral("下载文件"), QStringLiteral("正在下载游戏资源")},
        {QStringLiteral("校验文件"), QStringLiteral("下载完成后自动开始")},
        {QStringLiteral("就绪"), QStringLiteral("完成后可启动游戏")},
    }};
    return stages;
}

int installStageIndex(application::TaskPhase phase)
{
    switch (phase) {
    case application::TaskPhase::NotInstalled:
        return 0;
    case application::TaskPhase::Resolving:
        return 1;
    case application::TaskPhase::PreparingEnvironment:
        return 2;
    case application::TaskPhase::Downloading:
        return 3;
    case application::TaskPhase::Verifying:
        return 4;
    case application::TaskPhase::Ready:
    case application::TaskPhase::Committing:
        return 5;
    }
    return 0;
}

int installStageNumber(application::TaskPhase phase)
{
    return installStageIndex(phase) + 1;
}

QString installStageFailureText(application::TaskPhase phase)
{
    switch (phase) {
    case application::TaskPhase::NotInstalled:
    case application::TaskPhase::Ready:
        return QStringLiteral("安装失败");
    case application::TaskPhase::Resolving:
        return QStringLiteral("解析版本失败");
    case application::TaskPhase::PreparingEnvironment:
        return QStringLiteral("准备环境失败");
    case application::TaskPhase::Downloading:
        return QStringLiteral("下载失败");
    case application::TaskPhase::Verifying:
        return QStringLiteral("校验失败");
    case application::TaskPhase::Committing:
        return QStringLiteral("提交失败");
    }
    return QStringLiteral("安装失败");
}

} // namespace hustmcl::ui
