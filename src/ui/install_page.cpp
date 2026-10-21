#include "install_page.h"

#include "copy.h"
#include "elided_label.h"
#include "install_stages.h"
#include "page_header.h"
#include "primary_action_bar.h"
#include "stage_list.h"
#include "summary_row.h"

#include <QBoxLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace hustmcl::ui {

InstallPage::InstallPage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("安装游戏"), QStringLiteral("准备安装"), QString());

    m_content = new QWidget;
    auto* contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(0);

    m_columns = new QBoxLayout(QBoxLayout::LeftToRight);
    m_columns->setContentsMargins(0, 0, 0, 0);
    m_columns->setSpacing(24);

    m_left = new QWidget(m_content);
    m_left->setMaximumWidth(330);
    auto* leftLayout = new QVBoxLayout(m_left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);
    auto* stepsTitle = new QLabel(QStringLiteral("安装步骤"), m_left);
    stepsTitle->setObjectName(QStringLiteral("SectionTitle"));
    leftLayout->addWidget(stepsTitle);
    m_stageList = new StageList(m_left);
    leftLayout->addWidget(m_stageList);
    leftLayout->addStretch(1);

    m_divider = new QFrame(m_content);
    m_divider->setObjectName(QStringLiteral("VDivider"));
    m_divider->setFixedWidth(1);

    m_right = new QWidget(m_content);
    auto* rightLayout = new QVBoxLayout(m_right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    auto* installLabel = new QLabel(QStringLiteral("本次安装"), m_right);
    installLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(installLabel);
    rightLayout->addSpacing(6);

    auto* versionRow = new QHBoxLayout();
    versionRow->setContentsMargins(0, 0, 0, 0);
    versionRow->setSpacing(8);
    m_version = new QLabel(m_right);
    m_version->setObjectName(QStringLiteral("VersionLabel"));
    m_recommendedBadge = new QLabel(QStringLiteral("推荐"), m_right);
    m_recommendedBadge->setObjectName(QStringLiteral("BadgeRecommended"));
    m_verifiedBadge = new QLabel(QStringLiteral("已验证"), m_right);
    m_verifiedBadge->setObjectName(QStringLiteral("BadgeVerified"));
    versionRow->addWidget(m_version, 0, Qt::AlignBaseline);
    versionRow->addWidget(m_recommendedBadge, 0, Qt::AlignVCenter);
    versionRow->addWidget(m_verifiedBadge, 0, Qt::AlignVCenter);
    versionRow->addStretch(1);
    rightLayout->addLayout(versionRow);
    m_versionMeta = new QLabel(m_right);
    m_versionMeta->setObjectName(QStringLiteral("VersionMeta"));
    rightLayout->addWidget(m_versionMeta);

    auto* topDivider = new QFrame(m_right);
    topDivider->setObjectName(QStringLiteral("Divider"));
    topDivider->setFixedHeight(1);
    rightLayout->addSpacing(12);
    rightLayout->addWidget(topDivider);
    rightLayout->addSpacing(10);

    auto* pathLabel = new QLabel(QStringLiteral("安装目录"), m_right);
    pathLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(pathLabel);
    m_path = new ElidedLabel(m_right);
    m_path->setObjectName(QStringLiteral("InstallPath"));
    m_path->setFixedHeight(36);
    rightLayout->addWidget(m_path);
    auto* pathNote = new QLabel(QStringLiteral("安装中不可更改目录。"), m_right);
    pathNote->setObjectName(QStringLiteral("DetailNote"));
    rightLayout->addWidget(pathNote);
    rightLayout->addSpacing(12);

    auto* spaceLabel = new QLabel(QStringLiteral("空间"), m_right);
    spaceLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(spaceLabel);
    m_requiredRow = new SummaryRow(m_right);
    m_freeRow = new SummaryRow(m_right);
    rightLayout->addWidget(m_requiredRow);
    rightLayout->addWidget(m_freeRow);
    rightLayout->addSpacing(12);

    auto* middleDivider = new QFrame(m_right);
    middleDivider->setObjectName(QStringLiteral("Divider"));
    middleDivider->setFixedHeight(1);
    rightLayout->addSpacing(10);
    rightLayout->addWidget(middleDivider);
    rightLayout->addSpacing(10);

    auto* cancelLabel = new QLabel(QStringLiteral("取消与恢复"), m_right);
    cancelLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(cancelLabel);
    auto* cancelNote = new QLabel(
        QStringLiteral("取消后等待应用确认，保留已校验文件，不删除存档。"), m_right);
    cancelNote->setObjectName(QStringLiteral("DetailNote"));
    cancelNote->setWordWrap(true);
    rightLayout->addWidget(cancelNote);
    auto* retryNote = new QLabel(
        QStringLiteral("再次安装会从检查缓存开始。"), m_right);
    retryNote->setObjectName(QStringLiteral("DetailNote"));
    rightLayout->addWidget(retryNote);
    rightLayout->addSpacing(10);
    auto* completeLabel = new QLabel(QStringLiteral("完成后"), m_right);
    completeLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(completeLabel);
    auto* completeNote = new QLabel(
        QStringLiteral("安装就绪不等于已进入游戏主菜单；启动状态会单独显示。"), m_right);
    completeNote->setObjectName(QStringLiteral("DetailNote"));
    completeNote->setWordWrap(true);
    rightLayout->addWidget(completeNote);
    rightLayout->addStretch(1);

    m_columns->addWidget(m_left, 0);
    m_columns->addWidget(m_divider, 0);
    m_columns->addWidget(m_right, 1);
    contentLayout->addLayout(m_columns);
    setContent(m_content);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, [this]() {
        if (m_snapshot.task.retryAvailable) {
            emit retryRequested(m_snapshot.task.taskId);
        }
    });
    connect(m_actionBar, &PrimaryActionBar::secondaryClicked, this, [this]() {
        if (m_snapshot.task.cancelAvailable) {
            emit cancelRequested(m_snapshot.task.taskId);
        }
    });

    setSnapshot({});
    updateResponsiveLayout();
}

void InstallPage::setSnapshot(const application::InstallSnapshot& snapshot)
{
    m_snapshot = snapshot;
    const application::TaskSnapshot& task = snapshot.task;
    const QString instanceName = snapshot.instanceDisplayName.isEmpty()
        ? QStringLiteral("当前实例")
        : snapshot.instanceDisplayName;
    const QString subtitle = QStringLiteral("为%1准备 Minecraft %2")
                                 .arg(instanceName, snapshot.versionLabel);
    header()->setContent(QStringLiteral("安装游戏"), subtitle,
                         QStringLiteral("阶段 %1 / 6").arg(installStageNumber(task.phase)));

    m_version->setText(snapshot.versionLabel);
    m_recommendedBadge->setVisible(snapshot.recommended);
    m_verifiedBadge->setVisible(snapshot.verified);
    m_versionMeta->setText(snapshot.variantLabel);
    m_path->setFullText(snapshot.targetPath);
    m_requiredRow->setData(QStringLiteral("预计需要"),
                           snapshot.requiredBytes.has_value()
                               ? copy::formatBytes(snapshot.requiredBytes.value())
                               : QStringLiteral("未知"));
    m_freeRow->setData(QStringLiteral("当前可用"),
                       snapshot.freeBytes.has_value()
                           ? copy::formatBytes(snapshot.freeBytes.value())
                           : QStringLiteral("未知"));

    m_stageList->setTask(task);
    updateActionBar();
}

void InstallPage::focusPrimaryAction()
{
    if (!m_actionBar->primaryButton()->isHidden() && m_actionBar->primaryButton()->isEnabled()) {
        m_actionBar->primaryButton()->setFocus(Qt::OtherFocusReason);
        return;
    }
    if (!m_actionBar->secondaryButton()->isHidden() && m_actionBar->secondaryButton()->isEnabled()) {
        m_actionBar->secondaryButton()->setFocus(Qt::OtherFocusReason);
    }
}

void InstallPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void InstallPage::updateResponsiveLayout()
{
    if (!m_columns) {
        return;
    }
    const bool vertical = width() < 820;
    if (vertical) {
        m_columns->setDirection(QBoxLayout::TopToBottom);
        m_columns->setSpacing(20);
        m_divider->hide();
        m_left->setMaximumWidth(QWIDGETSIZE_MAX);
        m_left->setMinimumWidth(0);
        m_right->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        m_columns->setDirection(QBoxLayout::LeftToRight);
        m_columns->setSpacing(24);
        m_divider->show();
        m_left->setMaximumWidth(330);
        m_left->setMinimumWidth(280);
        m_right->setMaximumWidth(QWIDGETSIZE_MAX);
    }
}

void InstallPage::updateActionBar()
{
    const application::TaskSnapshot& task = m_snapshot.task;
    const bool failed = task.state == application::TaskState::Failed && task.error.has_value();
    m_actionBar->setErrorState(failed);

    QString title;
    QString note;
    QString primaryText;
    bool primaryVisible = false;
    bool primaryEnabled = false;
    QString primaryTip;
    QString secondaryText;
    bool secondaryVisible = false;
    bool secondaryEnabled = false;
    QString secondaryTip;

    switch (task.state) {
    case application::TaskState::Cancelling:
        title = QStringLiteral("正在取消…");
        note = QStringLiteral("正在等待应用确认并清理临时状态。");
        secondaryText = QStringLiteral("正在取消…");
        secondaryVisible = true;
        secondaryEnabled = false;
        secondaryTip = QStringLiteral("取消请求已发送，等待应用确认。");
        break;
    case application::TaskState::Failed:
        title = QStringLiteral("安装失败");
        note = task.error.has_value() ? task.error->summary : QStringLiteral("安装任务失败。");
        primaryText = QStringLiteral("重试");
        primaryVisible = task.retryAvailable;
        primaryEnabled = task.retryAvailable;
        primaryTip = task.retryReason;
        break;
    case application::TaskState::Cancelled:
        title = QStringLiteral("已取消");
        note = task.retryReason.isEmpty()
            ? QStringLiteral("已校验文件保留，可稍后继续。")
            : task.retryReason;
        primaryText = QStringLiteral("继续安装");
        primaryVisible = task.retryAvailable;
        primaryEnabled = task.retryAvailable;
        primaryTip = task.retryReason;
        break;
    case application::TaskState::Succeeded:
        title = QStringLiteral("安装就绪");
        note = QStringLiteral("安装完成不等于已进入游戏主菜单；启动状态会单独显示。");
        break;
    case application::TaskState::Running:
        if (task.phase == application::TaskPhase::Committing) {
            title = QStringLiteral("正在完成安装");
            note = task.cancelReason.isEmpty()
                ? QStringLiteral("提交窗口不可取消。")
                : task.cancelReason;
            secondaryText = QStringLiteral("取消不可用");
            secondaryVisible = true;
            secondaryEnabled = false;
            secondaryTip = note;
        } else {
            title = QStringLiteral("安装进行中");
            note = actionNote();
            if (task.cancelAvailable) {
                secondaryText = QStringLiteral("取消安装");
                secondaryVisible = true;
                secondaryEnabled = true;
                secondaryTip = QStringLiteral("请求取消当前安装任务。");
            } else if (!task.cancelReason.isEmpty()) {
                secondaryText = QStringLiteral("取消不可用");
                secondaryVisible = true;
                secondaryEnabled = false;
                secondaryTip = task.cancelReason;
            }
        }
        break;
    case application::TaskState::Idle:
        title = QStringLiteral("准备安装");
        note = QStringLiteral("等待应用开始安装任务。");
        break;
    }

    m_actionBar->setTitle(title);
    m_actionBar->setNote(note);
    m_actionBar->setPrimary(primaryText, primaryEnabled, primaryTip);
    m_actionBar->setPrimaryVisible(primaryVisible);
    m_actionBar->setSecondary(secondaryText, secondaryVisible, secondaryEnabled, secondaryTip, true);
    if (failed && task.error.has_value()) {
        m_actionBar->setToolTip(QStringLiteral("%1 [%2]\n%3")
                                    .arg(task.error->summary,
                                         copy::errorCodeText(task.error->code),
                                         task.error->detail));
    } else {
        m_actionBar->setToolTip({});
    }
}

QString InstallPage::actionNote() const
{
    const application::TaskSnapshot& task = m_snapshot.task;
    QString detail = task.phaseDetail;
    if (detail.isEmpty()) {
        const auto& stages = installStageDefinitions();
        detail = stages.at(static_cast<std::size_t>(installStageIndex(task.phase))).title;
    }
    if (task.totalBytes.has_value() && task.totalBytes.value() > 0) {
        return QStringLiteral("%1 · 已下载 %2 / %3 · %4")
            .arg(detail,
                 copy::formatBytes(task.completedBytes),
                 copy::formatBytes(task.totalBytes.value()),
                 copy::formatPercent(task.completedBytes, task.totalBytes.value()));
    }
    return QStringLiteral("%1 · 总量未知 · 已传输 %2")
        .arg(detail, copy::formatBytes(task.completedBytes));
}

} // namespace hustmcl::ui
