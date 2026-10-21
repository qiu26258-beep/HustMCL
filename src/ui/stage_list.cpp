#include "stage_list.h"

#include "install_stages.h"
#include "stage_row.h"

#include <QVBoxLayout>

namespace hustmcl::ui {

StageList::StageList(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    const auto& stages = installStageDefinitions();
    for (std::size_t i = 0; i < stages.size(); ++i) {
        auto* row = new StageRow(this);
        row->setObjectName(QStringLiteral("StageRow%1").arg(static_cast<int>(i)));
        row->setDetail(stages.at(i).title, stages.at(i).description);
        layout->addWidget(row, 0);
        m_rows.push_back(row);
    }
}

void StageList::setTask(const application::TaskSnapshot& task)
{
    const auto& stages = installStageDefinitions();
    const int current = installStageIndex(task.phase);
    for (std::size_t i = 0; i < m_rows.size(); ++i) {
        StageRow* row = m_rows.at(i);
        const bool done = static_cast<int>(i) < current;
        const bool isCurrent = static_cast<int>(i) == current;
        row->setStage(static_cast<int>(i) + 1, done, isCurrent,
                      i + 1 == m_rows.size());

        QString detail = done ? QStringLiteral("已完成") : stages.at(i).description;
        if (isCurrent) {
            if (task.state == application::TaskState::Cancelling) {
                detail = QStringLiteral("正在取消…");
            } else if (task.state == application::TaskState::Failed) {
                detail = installStageFailureText(task.phase);
            } else if (task.state == application::TaskState::Cancelled) {
                detail = QStringLiteral("已取消");
            } else if (task.phase == application::TaskPhase::Committing) {
                detail = QStringLiteral("正在完成安装");
            } else if (!task.phaseDetail.isEmpty()) {
                detail = task.phaseDetail;
            }
        }
        row->setDetail(stages.at(i).title, detail);

        const bool showProgress = isCurrent
            && task.phase == application::TaskPhase::Downloading
            && task.state != application::TaskState::Failed
            && task.state != application::TaskState::Cancelled;
        row->setProgress(task.completedBytes, task.totalBytes, showProgress);
    }
}

} // namespace hustmcl::ui
