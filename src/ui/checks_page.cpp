#include "checks_page.h"

#include "copy.h"
#include "elided_label.h"
#include "page_header.h"
#include "primary_action_bar.h"
#include "status_row.h"
#include "summary_row.h"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace hustmcl::ui {

ChecksPage::ChecksPage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("实例与检查"),
                         QStringLiteral("本地离线实例与启动前检查"), QString());

    m_content = new QWidget;
    auto* contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(0);

    m_columns = new QBoxLayout(QBoxLayout::LeftToRight);
    m_columns->setContentsMargins(0, 0, 0, 0);
    m_columns->setSpacing(24);

    m_left = new QWidget(m_content);
    auto* leftLayout = new QVBoxLayout(m_left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    auto* readinessTitle = new QLabel(QStringLiteral("启动前检查"), m_left);
    readinessTitle->setObjectName(QStringLiteral("SectionTitle"));
    leftLayout->addWidget(readinessTitle);
    leftLayout->addSpacing(4);
    auto* readinessNote = new QLabel(
        QStringLiteral("首次启动时会完整检查本地文件并检测 Java；"
                       "这里显示最近一次检查结果。"),
        m_left);
    readinessNote->setObjectName(QStringLiteral("DetailNote"));
    readinessNote->setWordWrap(true);
    leftLayout->addWidget(readinessNote);
    leftLayout->addSpacing(8);

    m_identityRow = new StatusRow(m_left);
    m_javaRow = new StatusRow(m_left);
    m_instanceRow = new StatusRow(m_left);
    m_fileCheckRow = new StatusRow(m_left);
    leftLayout->addWidget(m_identityRow);
    leftLayout->addWidget(m_javaRow);
    leftLayout->addWidget(m_instanceRow);
    leftLayout->addWidget(m_fileCheckRow);

    m_checkDetail = new QLabel(m_left);
    m_checkDetail->setObjectName(QStringLiteral("CheckDetail"));
    m_checkDetail->setWordWrap(true);
    leftLayout->addSpacing(4);
    leftLayout->addWidget(m_checkDetail);
    leftLayout->addSpacing(14);

    m_errorNotice = new QFrame(m_left);
    m_errorNotice->setObjectName(QStringLiteral("ErrorNotice"));
    auto* errorLayout = new QVBoxLayout(m_errorNotice);
    errorLayout->setContentsMargins(12, 10, 12, 10);
    errorLayout->setSpacing(4);
    m_errorCode = new QLabel(m_errorNotice);
    m_errorCode->setObjectName(QStringLiteral("ErrorTitle"));
    m_errorSummary = new QLabel(m_errorNotice);
    m_errorSummary->setObjectName(QStringLiteral("ErrorDetail"));
    m_errorSummary->setWordWrap(true);
    m_errorDetail = new QLabel(m_errorNotice);
    m_errorDetail->setObjectName(QStringLiteral("ErrorDetail"));
    m_errorDetail->setWordWrap(true);
    errorLayout->addWidget(m_errorCode);
    errorLayout->addWidget(m_errorSummary);
    errorLayout->addWidget(m_errorDetail);
    leftLayout->addWidget(m_errorNotice);
    leftLayout->addStretch(1);

    m_divider = new QFrame(m_content);
    m_divider->setObjectName(QStringLiteral("VDivider"));
    m_divider->setFixedWidth(1);

    m_right = new QWidget(m_content);
    m_right->setMaximumWidth(250);
    auto* rightLayout = new QVBoxLayout(m_right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    auto* detailTitle = new QLabel(QStringLiteral("本次启动"), m_right);
    detailTitle->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(detailTitle);
    rightLayout->addSpacing(8);
    m_versionRow = new SummaryRow(m_right);
    m_memoryRow = new SummaryRow(m_right);
    rightLayout->addWidget(m_versionRow);
    rightLayout->addWidget(m_memoryRow);
    rightLayout->addSpacing(12);

    auto* pathDivider = new QFrame(m_right);
    pathDivider->setObjectName(QStringLiteral("Divider"));
    pathDivider->setFixedHeight(1);
    rightLayout->addWidget(pathDivider);
    rightLayout->addSpacing(10);

    auto* dataLabel = new QLabel(QStringLiteral("数据目录"), m_right);
    dataLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(dataLabel);
    m_dataPath = new ElidedLabel(m_right);
    m_dataPath->setObjectName(QStringLiteral("PathValue"));
    m_dataPath->setFixedHeight(36);
    rightLayout->addWidget(m_dataPath);
    rightLayout->addSpacing(8);

    auto* logLabel = new QLabel(QStringLiteral("最近日志"), m_right);
    logLabel->setObjectName(QStringLiteral("DetailLabel"));
    rightLayout->addWidget(logLabel);
    m_logPath = new ElidedLabel(m_right);
    m_logPath->setObjectName(QStringLiteral("PathValue"));
    m_logPath->setFixedHeight(36);
    rightLayout->addWidget(m_logPath);
    rightLayout->addSpacing(12);

    auto* noteDivider = new QFrame(m_right);
    noteDivider->setObjectName(QStringLiteral("Divider"));
    noteDivider->setFixedHeight(1);
    rightLayout->addWidget(noteDivider);
    rightLayout->addSpacing(10);

    m_note = new QLabel(m_right);
    m_note->setObjectName(QStringLiteral("DetailNote"));
    m_note->setWordWrap(true);
    m_note->setText(
        QStringLiteral("准备阶段可以取消；游戏启动后请在游戏窗口内退出。"
                       "文件检查通过仅表示本地文件可用，不代表已进入游戏。"));
    rightLayout->addWidget(m_note);
    rightLayout->addStretch(1);

    m_columns->addWidget(m_left, 1);
    m_columns->addWidget(m_divider, 0);
    m_columns->addWidget(m_right, 0);
    contentLayout->addLayout(m_columns);
    setContent(m_content);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, [this]() {
        if (m_task.state == application::TaskState::Failed && m_task.retryAvailable) {
            emit retryRequested(m_task.taskId);
        }
    });
    connect(m_actionBar, &PrimaryActionBar::secondaryClicked, this, [this]() {
        if (m_task.cancelAvailable) {
            emit cancelRequested(m_task.taskId);
        }
    });

    setSnapshot({}, {});
    updateResponsiveLayout();
}

void ChecksPage::setSnapshot(const application::HomeSnapshot& home,
                             const application::TaskSnapshot& task)
{
    m_home = home;
    m_task = task;

    const QString instanceName = home.instance.displayName.isEmpty()
        ? QStringLiteral("本地离线实例")
        : home.instance.displayName;
    QString subtitle = QStringLiteral("%1 · 本地离线").arg(instanceName);
    if (!home.instance.versionLabel.isEmpty()) {
        subtitle = QStringLiteral("%1 · Minecraft %2 · 本地离线")
                       .arg(instanceName, home.instance.versionLabel);
    }
    header()->setContent(QStringLiteral("实例与检查"), subtitle,
                         home.statusTitle.isEmpty()
                             ? QStringLiteral("正在读取当前状态")
                             : home.statusTitle);

    const QString identityName = home.account.displayName.isEmpty()
        ? QStringLiteral("未设置")
        : home.account.displayName;
    m_identityRow->setData(
        QStringLiteral("离线身份"), identityName,
        copy::accountStateText(home.account.state),
        home.account.state == application::AccountState::Offline);

    QString javaValue = home.environment.versionLabel;
    if (!home.environment.architectureLabel.isEmpty()) {
        javaValue += QStringLiteral(" · ") + home.environment.architectureLabel;
    }
    if (javaValue.isEmpty()) {
        javaValue = QStringLiteral("启动时检测");
    }
    m_javaRow->setData(QStringLiteral("Java 运行时"), javaValue,
                       copy::javaStateText(home.environment.state),
                       home.environment.state == application::JavaState::Ready);

    QString instanceValue = home.instance.versionLabel;
    if (!home.instance.editionLabel.isEmpty()) {
        instanceValue += QStringLiteral(" · ") + home.instance.editionLabel;
    }
    if (instanceValue.isEmpty()) {
        instanceValue = QStringLiteral("启动时读取");
    }
    m_instanceRow->setData(QStringLiteral("游戏实例"), instanceValue,
                           copy::instanceStateText(home.instance.state),
                           home.instance.state == application::InstanceState::Ready);

    const bool checkPassed =
        home.instance.state == application::InstanceState::Ready;
    m_fileCheckRow->setData(
        QStringLiteral("文件校验"),
        checkPassed ? QStringLiteral("本地文件完整") : copy::checkPhaseText(task.phase),
        checkPassed ? QStringLiteral("已通过") : copy::taskStateText(task.state),
        checkPassed);
    QString detail = task.phaseDetail;
    if (detail.isEmpty()) {
        detail = task.state == application::TaskState::Idle
            ? QStringLiteral("尚未检查本地文件。点击启动后会先完整检查本地文件与 Java。")
            : QStringLiteral("已检查的文件会保留；失败或取消后可以重试。");
    }
    m_checkDetail->setText(detail);

    const bool hasError = task.error.has_value();
    m_errorNotice->setVisible(hasError);
    if (hasError) {
        const application::TaskError& error = task.error.value();
        m_errorCode->setText(QStringLiteral("%1 · %2")
                                 .arg(copy::errorCodeText(error.code),
                                      error.summary.isEmpty()
                                          ? QStringLiteral("启动准备失败")
                                          : error.summary));
        m_errorSummary->setText(error.detail);
        m_errorSummary->setVisible(!error.detail.isEmpty());
        m_errorDetail->setText(
            QStringLiteral("可重试：%1 · 详细错误已记录到日志页。")
                .arg(error.retryable ? QStringLiteral("是") : QStringLiteral("否")));
    }

    m_versionRow->setData(QStringLiteral("游戏版本"),
                          home.instance.versionLabel.isEmpty()
                              ? QStringLiteral("未知")
                              : home.instance.versionLabel);
    m_memoryRow->setData(QStringLiteral("内存上限"),
                         copy::formatMemory(home.launch.memoryMb));
    m_dataPath->setFullText(home.dataPath);
    m_logPath->setFullText(home.logPath);
    m_dataPath->setToolTip(home.dataPath.isEmpty()
                               ? QStringLiteral("正在读取数据目录…")
                               : home.dataPath);
    m_logPath->setToolTip(home.logPath.isEmpty()
                              ? QStringLiteral("还没有运行日志；启动游戏后生成。")
                              : home.logPath);
    if (home.logPath.isEmpty()) {
        m_logPath->setFullText(QStringLiteral("还没有运行日志；启动游戏后生成。"));
    }

    updateActionBar();
}

void ChecksPage::focusPrimaryAction()
{
    if (!m_actionBar->primaryButton()->isHidden() && m_actionBar->primaryButton()->isEnabled()) {
        m_actionBar->primaryButton()->setFocus(Qt::OtherFocusReason);
        return;
    }
    if (!m_actionBar->secondaryButton()->isHidden() && m_actionBar->secondaryButton()->isEnabled()) {
        m_actionBar->secondaryButton()->setFocus(Qt::OtherFocusReason);
    }
}

void ChecksPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void ChecksPage::updateResponsiveLayout()
{
    if (!m_columns) {
        return;
    }
    const bool vertical = width() < 820;
    if (vertical) {
        m_columns->setDirection(QBoxLayout::TopToBottom);
        m_columns->setSpacing(20);
        m_divider->hide();
        m_right->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        m_columns->setDirection(QBoxLayout::LeftToRight);
        m_columns->setSpacing(24);
        m_divider->show();
        m_right->setMaximumWidth(250);
    }
}

void ChecksPage::updateActionBar()
{
    const QString title = m_home.statusTitle.isEmpty()
        ? QStringLiteral("启动前检查")
        : m_home.statusTitle;
    m_actionBar->setTitle(title);

    QString note = m_home.primaryReason;
    if (note.isEmpty()) {
        note = m_home.sessionActive
            ? QStringLiteral("游戏运行中。关闭启动器窗口不会结束游戏。")
            : QStringLiteral("启动操作在首页；首次启动会完整检查文件与 Java。");
    }
    m_actionBar->setNote(note);

    const bool canRetry = m_task.state == application::TaskState::Failed
        && m_task.retryAvailable;
    if (canRetry) {
        m_actionBar->setPrimary(
            QStringLiteral("重试启动"), true,
            m_task.retryReason.isEmpty()
                ? QStringLiteral("重新检查本地文件与 Java 并启动游戏。")
                : m_task.retryReason);
        m_actionBar->setPrimaryVisible(true);
    } else {
        m_actionBar->setPrimary(QString(), false, {});
        m_actionBar->setPrimaryVisible(false);
    }
    if (m_task.cancelAvailable) {
        m_actionBar->setSecondary(
            QStringLiteral("取消准备"), true, true,
            QStringLiteral("取消当前启动准备，不会删除游戏文件或存档。"), true);
    } else {
        m_actionBar->setSecondary(QString(), false, false, {});
    }
    m_actionBar->setErrorState(m_task.error.has_value());
}

} // namespace hustmcl::ui
