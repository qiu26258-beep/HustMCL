#include "stage_row.h"

#include "copy.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStyle>
#include <QVBoxLayout>

namespace hustmcl::ui {

StageRow::StageRow(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("StageRow"));
    setMinimumHeight(44);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto* indicator = new QWidget(this);
    indicator->setFixedWidth(27);
    auto* indicatorLayout = new QVBoxLayout(indicator);
    indicatorLayout->setContentsMargins(0, 0, 0, 0);
    indicatorLayout->setSpacing(0);
    m_dot = new QLabel(indicator);
    m_dot->setObjectName(QStringLiteral("StageDot"));
    m_dot->setProperty("stageState", QStringLiteral("future"));
    m_dot->setAlignment(Qt::AlignCenter);
    m_line = new QFrame(indicator);
    m_line->setObjectName(QStringLiteral("StageLine"));
    m_line->setFixedWidth(1);
    m_line->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    indicatorLayout->addWidget(m_dot, 0, Qt::AlignHCenter);
    indicatorLayout->addWidget(m_line, 1, Qt::AlignHCenter);
    layout->addWidget(indicator, 0, Qt::AlignTop);

    auto* content = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 2, 0, 10);
    contentLayout->setSpacing(4);
    m_title = new QLabel(content);
    m_title->setObjectName(QStringLiteral("StageTitle"));
    m_title->setProperty("stageState", QStringLiteral("future"));
    m_detail = new QLabel(content);
    m_detail->setObjectName(QStringLiteral("StageSub"));
    m_detail->setProperty("stageState", QStringLiteral("future"));
    m_detail->setWordWrap(true);
    contentLayout->addWidget(m_title);
    contentLayout->addWidget(m_detail);

    m_progressBox = new QWidget(content);
    auto* progressLayout = new QVBoxLayout(m_progressBox);
    progressLayout->setContentsMargins(0, 2, 0, 0);
    progressLayout->setSpacing(4);
    m_progress = new QProgressBar(m_progressBox);
    m_progress->setObjectName(QStringLiteral("StageProgress"));
    m_progress->setTextVisible(false);
    m_progress->setRange(0, 1000);
    m_progressText = new QLabel(m_progressBox);
    m_progressText->setObjectName(QStringLiteral("ProgressText"));
    m_progressText->setWordWrap(true);
    m_unknown = new QFrame(m_progressBox);
    m_unknown->setObjectName(QStringLiteral("UnknownNote"));
    auto* unknownLayout = new QHBoxLayout(m_unknown);
    unknownLayout->setContentsMargins(9, 6, 9, 6);
    unknownLayout->setSpacing(8);
    m_unknownTitle = new QLabel(QStringLiteral("总量未知"), m_unknown);
    m_unknownTitle->setObjectName(QStringLiteral("UnknownTitle"));
    m_unknownValue = new QLabel(m_unknown);
    m_unknownValue->setObjectName(QStringLiteral("UnknownValue"));
    unknownLayout->addWidget(m_unknownTitle, 0);
    unknownLayout->addWidget(m_unknownValue, 1);
    progressLayout->addWidget(m_progress);
    progressLayout->addWidget(m_progressText);
    progressLayout->addWidget(m_unknown);
    contentLayout->addWidget(m_progressBox);
    m_progressBox->hide();

    layout->addWidget(content, 1);
}

void StageRow::setStage(int number, bool done, bool current, bool last)
{
    const QString state = done ? QStringLiteral("done")
                               : (current ? QStringLiteral("current") : QStringLiteral("future"));
    m_dot->setText(done ? QStringLiteral("✓") : QString::number(number));
    setStageProperty(m_dot, state);
    setStageProperty(m_title, state);
    setStageProperty(m_detail, state);
    m_line->setVisible(!last);
}

void StageRow::setDetail(const QString& title, const QString& detail)
{
    m_title->setText(title);
    m_detail->setText(detail);
    m_detail->setToolTip(detail);
    setAccessibleName(QStringLiteral("%1，%2").arg(title, detail));
}

void StageRow::setProgress(qint64 completedBytes, std::optional<qint64> totalBytes, bool visible)
{
    m_progressBox->setVisible(visible);
    if (!visible) {
        return;
    }
    if (totalBytes.has_value() && totalBytes.value() > 0) {
        const qint64 total = totalBytes.value();
        const int value = static_cast<int>(qRound(copy::progressFraction(completedBytes, total) * 1000.0));
        m_progress->setValue(value);
        m_progress->show();
        m_progressText->setText(QStringLiteral("已下载 %1 / %2 · %3")
                                    .arg(copy::formatBytes(completedBytes),
                                         copy::formatBytes(total),
                                         copy::formatPercent(completedBytes, total)));
        m_progressText->show();
        m_unknown->hide();
    } else {
        m_progress->hide();
        m_progressText->hide();
        m_unknownValue->setText(QStringLiteral("已传输 %1").arg(copy::formatBytes(completedBytes)));
        m_unknown->show();
    }
}

void StageRow::hideProgress()
{
    m_progressBox->hide();
}

void StageRow::setStageProperty(QLabel* label, const QString& value)
{
    label->setProperty("stageState", value);
    repolish(label);
}

void StageRow::repolish(QLabel* label)
{
    label->style()->unpolish(label);
    label->style()->polish(label);
    label->update();
}

} // namespace hustmcl::ui
