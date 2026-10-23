#include "logs_page.h"

#include "copy.h"
#include "elided_label.h"
#include "page_header.h"
#include "primary_action_bar.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace hustmcl::ui {

LogsPage::LogsPage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("日志与错误"),
                         QStringLiteral("最近一次启动状态、错误与日志位置"),
                         QStringLiteral("本地离线"));

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(0);

    auto* statusLabel = new QLabel(QStringLiteral("当前状态"), content);
    statusLabel->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(statusLabel);
    layout->addSpacing(6);
    m_statusTitle = new QLabel(content);
    m_statusTitle->setObjectName(QStringLiteral("LogStatusTitle"));
    layout->addWidget(m_statusTitle);
    m_statusDetail = new QLabel(content);
    m_statusDetail->setObjectName(QStringLiteral("DetailNote"));
    m_statusDetail->setWordWrap(true);
    layout->addWidget(m_statusDetail);
    m_identity = new QLabel(content);
    m_identity->setObjectName(QStringLiteral("DetailLabel"));
    layout->addWidget(m_identity);

    auto* divider = new QFrame(content);
    divider->setObjectName(QStringLiteral("Divider"));
    divider->setFixedHeight(1);
    layout->addSpacing(16);
    layout->addWidget(divider);
    layout->addSpacing(14);

    auto* errorLabel = new QLabel(QStringLiteral("最近错误"), content);
    errorLabel->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(errorLabel);
    layout->addSpacing(6);
    m_noError = new QLabel(QStringLiteral("最近没有错误。启动准备或游戏启动失败后，"
                                          "这里会显示错误码和原因。"),
                           content);
    m_noError->setObjectName(QStringLiteral("DetailNote"));
    m_noError->setWordWrap(true);
    layout->addWidget(m_noError);

    m_errorNotice = new QFrame(content);
    m_errorNotice->setObjectName(QStringLiteral("ErrorNotice"));
    auto* errorLayout = new QVBoxLayout(m_errorNotice);
    errorLayout->setContentsMargins(12, 10, 12, 10);
    errorLayout->setSpacing(4);
    m_errorTitle = new QLabel(m_errorNotice);
    m_errorTitle->setObjectName(QStringLiteral("ErrorTitle"));
    m_errorTitle->setWordWrap(true);
    m_errorSummary = new QLabel(m_errorNotice);
    m_errorSummary->setObjectName(QStringLiteral("ErrorDetail"));
    m_errorSummary->setWordWrap(true);
    m_errorDetail = new QLabel(m_errorNotice);
    m_errorDetail->setObjectName(QStringLiteral("ErrorDetail"));
    m_errorDetail->setWordWrap(true);
    errorLayout->addWidget(m_errorTitle);
    errorLayout->addWidget(m_errorSummary);
    errorLayout->addWidget(m_errorDetail);
    layout->addWidget(m_errorNotice);

    layout->addSpacing(16);
    auto* pathDivider = new QFrame(content);
    pathDivider->setObjectName(QStringLiteral("Divider"));
    pathDivider->setFixedHeight(1);
    layout->addWidget(pathDivider);
    layout->addSpacing(14);

    auto* pathTitle = new QLabel(QStringLiteral("日志位置"), content);
    pathTitle->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(pathTitle);
    layout->addSpacing(6);
    m_logPath = new ElidedLabel(content);
    m_logPath->setObjectName(QStringLiteral("PathValue"));
    m_logPath->setFixedHeight(36);
    layout->addWidget(m_logPath);
    auto* logNote = new QLabel(
        QStringLiteral("本次运行的日志文件保存在该位置；游戏启动后才会生成。"),
        content);
    logNote->setObjectName(QStringLiteral("DetailNote"));
    logNote->setWordWrap(true);
    layout->addWidget(logNote);
    layout->addSpacing(10);

    auto* dataLabel = new QLabel(QStringLiteral("数据目录"), content);
    dataLabel->setObjectName(QStringLiteral("DetailLabel"));
    layout->addWidget(dataLabel);
    m_dataPath = new ElidedLabel(content);
    m_dataPath->setObjectName(QStringLiteral("PathValue"));
    m_dataPath->setFixedHeight(36);
    layout->addWidget(m_dataPath);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 12, 0, 0);
    buttonRow->setSpacing(10);
    m_openLogs = new QPushButton(QStringLiteral("打开日志文件夹"), content);
    m_openLogs->setObjectName(QStringLiteral("DialogAction"));
    m_openLogs->setCursor(Qt::PointingHandCursor);
    m_openLogs->setAutoDefault(false);
    m_openLogs->setAccessibleName(QStringLiteral("打开日志文件夹"));
    m_openSaves = new QPushButton(QStringLiteral("打开游戏存档文件夹"), content);
    m_openSaves->setObjectName(QStringLiteral("DialogAction"));
    m_openSaves->setCursor(Qt::PointingHandCursor);
    m_openSaves->setAutoDefault(false);
    m_openSaves->setAccessibleName(QStringLiteral("打开游戏存档文件夹"));
    buttonRow->addWidget(m_openLogs);
    buttonRow->addWidget(m_openSaves);
    buttonRow->addStretch(1);
    layout->addLayout(buttonRow);
    auto* buttonNote = new QLabel(
        QStringLiteral("在此查看本次启动状态；打开日志文件夹可获取详细记录。"),
        content);
    buttonNote->setObjectName(QStringLiteral("DetailNote"));
    buttonNote->setWordWrap(true);
    layout->addWidget(buttonNote);
    layout->addStretch(1);

    setContent(content);

    m_actionBar = actionBar();
    m_actionBar->setTitle(QStringLiteral("运行记录"));
    m_actionBar->setNote(
        QStringLiteral("这里显示最近一次启动状态与错误；详细记录请打开日志文件夹。"));
    m_actionBar->setPrimary(QString(), false, {});
    m_actionBar->setPrimaryVisible(false);
    m_actionBar->setSecondary(QString(), false, false, {});

    connect(m_openLogs, &QPushButton::clicked, this, &LogsPage::openLogFolderRequested);
    connect(m_openSaves, &QPushButton::clicked, this, &LogsPage::openSavesFolderRequested);

    setSnapshot({}, {});
}

void LogsPage::setSnapshot(const application::HomeSnapshot& home,
                           const application::TaskSnapshot& task)
{
    m_home = home;
    const QString status = home.statusTitle.isEmpty()
        ? (home.sessionActive ? QStringLiteral("游戏运行中") : QStringLiteral("本地离线待启动"))
        : home.statusTitle;
    header()->setContent(
        QStringLiteral("日志与错误"),
        QStringLiteral("最近一次启动状态、错误与日志位置"),
        home.sessionActive ? QStringLiteral("游戏运行中") : status);

    m_statusTitle->setText(status);
    m_statusDetail->setText(home.primaryReason.isEmpty()
                                ? QStringLiteral("正在读取当前状态。")
                                : home.primaryReason);
    const QString name = home.account.displayName.isEmpty()
        ? QStringLiteral("本地玩家")
        : home.account.displayName;
    m_identity->setText(
        QStringLiteral("离线身份：%1 · %2")
            .arg(name, copy::accountStateText(home.account.state)));

    const bool hasError = task.error.has_value();
    m_errorNotice->setVisible(hasError);
    m_noError->setVisible(!hasError);
    if (hasError) {
        const application::TaskError& error = task.error.value();
        m_errorTitle->setText(
            QStringLiteral("错误码 %1 · %2")
                .arg(copy::errorCodeText(error.code),
                     error.summary.isEmpty() ? QStringLiteral("未知错误") : error.summary));
        m_errorSummary->setText(error.detail);
        m_errorSummary->setVisible(!error.detail.isEmpty());
        m_errorDetail->setText(
            QStringLiteral("阶段：%1 · 可重试：%2")
                .arg(copy::checkPhaseText(task.phase),
                     error.retryable ? QStringLiteral("是") : QStringLiteral("否")));
    }

    const bool hasLog = !home.logPath.isEmpty();
    m_logPath->setFullText(hasLog
                               ? home.logPath
                               : QStringLiteral("还没有运行日志；首次启动游戏后生成。"));
    m_logPath->setToolTip(hasLog
                              ? home.logPath
                              : QStringLiteral("还没有运行日志；首次启动游戏后在此显示。"));
    m_dataPath->setFullText(home.dataPath.isEmpty()
                                ? QStringLiteral("正在读取数据目录…")
                                : home.dataPath);
    m_dataPath->setToolTip(home.dataPath);
    m_openLogs->setEnabled(hasLog);
    m_openLogs->setToolTip(hasLog
                               ? QStringLiteral("打开包含本次运行日志的文件夹。")
                               : QStringLiteral("还没有日志；启动游戏后才能打开。"));
    m_openSaves->setEnabled(!home.dataPath.isEmpty());
    m_openSaves->setToolTip(home.dataPath.isEmpty()
                                ? QStringLiteral("正在读取数据目录。")
                                : QStringLiteral("打开本地游戏存档文件夹。"));
}

void LogsPage::focusPrimaryAction()
{
    if (m_openLogs->isEnabled()) {
        m_openLogs->setFocus(Qt::OtherFocusReason);
    } else if (m_openSaves->isEnabled()) {
        m_openSaves->setFocus(Qt::OtherFocusReason);
    }
}

} // namespace hustmcl::ui
