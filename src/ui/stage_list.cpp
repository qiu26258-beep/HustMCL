#include "stage_list.h"

#include "stage_row.h"

#include <QVBoxLayout>

namespace hustmcl::ui {

StageList::StageList(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    const QStringList titles = {
        QStringLiteral("未安装"),
        QStringLiteral("解析版本"),
        QStringLiteral("准备环境"),
        QStringLiteral("下载文件"),
        QStringLiteral("校验文件"),
        QStringLiteral("就绪"),
    };
    const QStringList descriptions = {
        QStringLiteral("目标目录为空"),
        QStringLiteral("文件清单已确认"),
        QStringLiteral("Java 运行时已就绪"),
        QStringLiteral("正在下载游戏资源"),
        QStringLiteral("下载完成后自动开始"),
        QStringLiteral("完成后可启动游戏"),
    };
    for (int i = 0; i < titles.size(); ++i) {
        auto* row = new StageRow(this);
        row->setObjectName(QStringLiteral("StageRow%1").arg(i));
        row->setDetail(titles.at(i), descriptions.at(i));
        layout->addWidget(row, 0);
        m_rows.push_back(row);
    }
}

void StageList::setTask(const application::TaskSnapshot& task)
{
    const QStringList titles = {
        QStringLiteral("未安装"),
        QStringLiteral("解析版本"),
        QStringLiteral("准备环境"),
        QStringLiteral("下载文件"),
        QStringLiteral("校验文件"),
        QStringLiteral("就绪"),
    };
    const QStringList descriptions = {
        QStringLiteral("目标目录为空"),
        QStringLiteral("文件清单已确认"),
        QStringLiteral("Java 运行时已就绪"),
        QStringLiteral("正在下载游戏资源"),
        QStringLiteral("下载完成后自动开始"),
        QStringLiteral("完成后可启动游戏"),
    };

    const int current = phaseIndex(task.phase);
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        StageRow* row = m_rows.at(static_cast<std::size_t>(i));
        const bool done = i < current;
        const bool isCurrent = i == current;
        row->setStage(i + 1, done, isCurrent, i == static_cast<int>(m_rows.size()) - 1);

        QString detail = done ? QStringLiteral("已完成") : descriptions.at(i);
        if (isCurrent) {
            if (task.state == application::TaskState::Cancelling) {
                detail = QStringLiteral("正在取消…");
            } else if (task.state == application::TaskState::Failed) {
                detail = QStringLiteral("下载失败");
            } else if (task.state == application::TaskState::Cancelled) {
                detail = QStringLiteral("已取消");
            } else if (task.phase == application::TaskPhase::Committing) {
                detail = QStringLiteral("正在完成安装");
            } else if (!task.phaseDetail.isEmpty()) {
                detail = task.phaseDetail;
            }
        }
        row->setDetail(titles.at(i), detail);

        const bool showProgress = isCurrent
            && task.phase == application::TaskPhase::Downloading
            && task.state != application::TaskState::Failed
            && task.state != application::TaskState::Cancelled;
        row->setProgress(task.completedBytes, task.totalBytes, showProgress);
    }
}

int StageList::phaseIndex(application::TaskPhase phase)
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

} // namespace hustmcl::ui
