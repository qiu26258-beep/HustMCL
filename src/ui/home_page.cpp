#include "home_page.h"

#include "campus_artwork.h"
#include "copy.h"
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

HomePage::HomePage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("下午好"), QStringLiteral("等待应用状态"), QString());
    header()->setTrailingText(QString());

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

    m_instanceName = new QLabel(m_left);
    m_instanceName->setObjectName(QStringLiteral("InstanceName"));
    m_instanceMeta = new QLabel(m_left);
    m_instanceMeta->setObjectName(QStringLiteral("InstanceMeta"));
    leftLayout->addWidget(m_instanceName);
    leftLayout->addWidget(m_instanceMeta);

    auto* topDivider = new QFrame(m_left);
    topDivider->setObjectName(QStringLiteral("Divider"));
    topDivider->setFixedHeight(1);
    leftLayout->addSpacing(18);
    leftLayout->addWidget(topDivider);
    leftLayout->addSpacing(18);

    auto* readinessTitle = new QLabel(QStringLiteral("准备状态"), m_left);
    readinessTitle->setObjectName(QStringLiteral("SectionTitle"));
    leftLayout->addWidget(readinessTitle);
    leftLayout->addSpacing(8);
    m_accountRow = new StatusRow(m_left);
    m_javaRow = new StatusRow(m_left);
    m_instanceRow = new StatusRow(m_left);
    leftLayout->addWidget(m_accountRow);
    leftLayout->addWidget(m_javaRow);
    leftLayout->addWidget(m_instanceRow);
    leftLayout->addSpacing(16);

    auto* summaryTitle = new QLabel(QStringLiteral("启动摘要"), m_left);
    summaryTitle->setObjectName(QStringLiteral("SectionTitle"));
    leftLayout->addWidget(summaryTitle);
    leftLayout->addSpacing(6);
    m_memoryRow = new SummaryRow(m_left);
    m_launchRow = new SummaryRow(m_left);
    leftLayout->addWidget(m_memoryRow);
    leftLayout->addWidget(m_launchRow);
    leftLayout->addSpacing(10);
    m_footnote = new QLabel(m_left);
    m_footnote->setObjectName(QStringLiteral("Footnote"));
    m_footnote->setWordWrap(true);
    leftLayout->addWidget(m_footnote);
    leftLayout->addStretch(1);

    m_divider = new QFrame(m_content);
    m_divider->setObjectName(QStringLiteral("VDivider"));
    m_divider->setFixedWidth(1);

    m_right = new QWidget(m_content);
    m_right->setMaximumWidth(226);
    auto* rightLayout = new QVBoxLayout(m_right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);
    m_artwork = new CampusArtwork(m_right);
    m_artwork->setFixedSize(226, 280);
    rightLayout->addWidget(m_artwork, 0, Qt::AlignHCenter);
    m_artCaption = new QLabel(QStringLiteral("小科陪你，开启新世界。"), m_right);
    m_artCaption->setObjectName(QStringLiteral("SidebarCaption"));
    m_artCaption->setWordWrap(true);
    m_artSub = new QLabel(QStringLiteral("树荫下的片刻，也是冒险的起点。"), m_right);
    m_artSub->setObjectName(QStringLiteral("SidebarSub"));
    m_artSub->setWordWrap(true);
    rightLayout->addWidget(m_artCaption);
    rightLayout->addWidget(m_artSub);
    rightLayout->addSpacing(6);
    m_settingsLink = new QPushButton(QStringLiteral("打开设置 →"), m_right);
    m_settingsLink->setObjectName(QStringLiteral("SecondaryAction"));
    m_settingsLink->setCursor(Qt::PointingHandCursor);
    m_settingsLink->setAutoDefault(false);
    m_settingsLink->setAccessibleName(QStringLiteral("打开设置"));
    m_logsLink = new QPushButton(QStringLiteral("日志与错误 →"), m_right);
    m_logsLink->setObjectName(QStringLiteral("SecondaryAction"));
    m_logsLink->setCursor(Qt::PointingHandCursor);
    m_logsLink->setAutoDefault(false);
    m_logsLink->setAccessibleName(QStringLiteral("日志与错误"));
    rightLayout->addWidget(m_settingsLink, 0, Qt::AlignLeft);
    rightLayout->addWidget(m_logsLink, 0, Qt::AlignLeft);
    rightLayout->addStretch(1);

    m_columns->addWidget(m_left, 1);
    m_columns->addWidget(m_divider, 0);
    m_columns->addWidget(m_right, 0);
    contentLayout->addLayout(m_columns);
    setContent(m_content);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, &HomePage::primaryActionRequested);
    connect(m_actionBar, &PrimaryActionBar::secondaryClicked, this, &HomePage::installRequested);
    connect(m_settingsLink, &QPushButton::clicked, this, [this]() {
        emit unimplementedFeatureRequested(QStringLiteral("设置页"));
    });
    connect(m_logsLink, &QPushButton::clicked, this, [this]() {
        emit unimplementedFeatureRequested(QStringLiteral("日志与错误页"));
    });

    setSnapshot({});
    updateResponsiveLayout();
}

void HomePage::setSnapshot(const application::HomeSnapshot& snapshot)
{
    const QString accountName = snapshot.account.displayName.isEmpty()
        ? QStringLiteral("尚未登录")
        : snapshot.account.displayName;
    const QString greeting = snapshot.greeting.isEmpty()
        ? QStringLiteral("你好")
        : snapshot.greeting;
    const QString instanceName = snapshot.instance.displayName.isEmpty()
        ? QStringLiteral("尚未安装实例")
        : snapshot.instance.displayName;
    QString instanceMeta = snapshot.instance.versionLabel;
    if (!snapshot.instance.editionLabel.isEmpty()) {
        instanceMeta += QStringLiteral(" · ") + snapshot.instance.editionLabel;
    }
    if (instanceMeta.isEmpty()) {
        instanceMeta = QStringLiteral("等待安装");
    }
    QString headerSubtitle = instanceName;
    if (!snapshot.instance.versionLabel.isEmpty()) {
        headerSubtitle = QStringLiteral("%1 · Minecraft %2")
                             .arg(instanceName, snapshot.instance.versionLabel);
    }
    header()->setContent(greeting, headerSubtitle, snapshot.timeLabel);
    m_instanceName->setText(instanceName);
    m_instanceMeta->setText(instanceMeta);

    QString accountValue = accountName;
    if (!snapshot.account.providerLabel.isEmpty()) {
        accountValue += QStringLiteral(" · ") + snapshot.account.providerLabel;
    }
    m_accountRow->setData(QStringLiteral("账号"), accountValue,
                          copy::accountStateText(snapshot.account.state),
                          snapshot.account.state == application::AccountState::SignedIn);

    QString javaValue = snapshot.environment.versionLabel;
    if (!snapshot.environment.architectureLabel.isEmpty()) {
        javaValue += QStringLiteral(" · ") + snapshot.environment.architectureLabel;
    }
    if (javaValue.isEmpty()) {
        javaValue = QStringLiteral("由应用选择");
    }
    m_javaRow->setData(QStringLiteral("Java 运行时"), javaValue,
                       copy::javaStateText(snapshot.environment.state),
                       snapshot.environment.state == application::JavaState::Ready);

    QString instanceValue = snapshot.instance.versionLabel;
    if (!snapshot.instance.editionLabel.isEmpty()) {
        instanceValue += QStringLiteral(" · ") + snapshot.instance.editionLabel;
    }
    if (instanceValue.isEmpty()) {
        instanceValue = QStringLiteral("尚未安装");
    }
    m_instanceRow->setData(QStringLiteral("游戏实例"), instanceValue,
                           copy::instanceStateText(snapshot.instance.state),
                           snapshot.instance.state == application::InstanceState::Ready);

    m_memoryRow->setData(QStringLiteral("内存上限"), copy::formatMemory(snapshot.launch.memoryMb));
    m_launchRow->setData(QStringLiteral("启动后"),
                         snapshot.launch.closeLauncherAfterLaunch
                             ? QStringLiteral("关闭启动器")
                             : QStringLiteral("保持启动器打开"));
    m_footnote->setText(snapshot.launch.closeLauncherAfterLaunch
                            ? QStringLiteral("启动后可以关闭启动器，游戏会继续运行。")
                            : QStringLiteral("启动后启动器会保持打开。"));

    const QString primaryText = copy::primaryActionText(snapshot.primaryAction);
    m_actionBar->setTitle(actionTitle(snapshot));
    m_actionBar->setNote(snapshot.primaryReason.isEmpty()
                             ? QStringLiteral("等待应用提供下一步。")
                             : snapshot.primaryReason);
    m_actionBar->setPrimary(primaryText, snapshot.primaryEnabled, snapshot.primaryReason);
    m_actionBar->setPrimaryVisible(!primaryText.isEmpty());
    m_actionBar->setSecondary(QStringLiteral("安装其他版本"), true, true,
                             QStringLiteral("打开安装页"));
    m_actionBar->setErrorState(false);
}

void HomePage::setDecorationsVisible(bool visible)
{
    PageScaffold::setDecorationsVisible(visible);
    m_artwork->setVisible(visible);
    m_artCaption->setVisible(visible);
    m_artSub->setVisible(visible);
}

void HomePage::focusPrimaryAction()
{
    if (!m_actionBar->primaryButton()->isHidden() && m_actionBar->primaryButton()->isEnabled()) {
        m_actionBar->primaryButton()->setFocus(Qt::OtherFocusReason);
        return;
    }
    if (!m_actionBar->secondaryButton()->isHidden()) {
        m_actionBar->secondaryButton()->setFocus(Qt::OtherFocusReason);
    }
}

void HomePage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void HomePage::updateResponsiveLayout()
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
        m_artwork->setFixedSize(226, 280);
    } else {
        m_columns->setDirection(QBoxLayout::LeftToRight);
        m_columns->setSpacing(24);
        m_divider->show();
        m_right->setMaximumWidth(226);
        m_artwork->setFixedSize(226, 280);
    }
}

QString HomePage::actionTitle(const application::HomeSnapshot& snapshot) const
{
    if (snapshot.primaryAction == application::PrimaryAction::None) {
        return QStringLiteral("暂无可用操作");
    }
    if (snapshot.primaryAction == application::PrimaryAction::Launch) {
        return QStringLiteral("准备就绪");
    }
    return QStringLiteral("下一步");
}

} // namespace hustmcl::ui
