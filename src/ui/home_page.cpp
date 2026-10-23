#include "home_page.h"

#include "campus_artwork.h"
#include "copy.h"
#include "page_header.h"
#include "primary_action_bar.h"
#include "status_row.h"
#include "summary_row.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace hustmcl::ui {

namespace {

QString importPhaseText(application::ImportPhase phase)
{
    switch (phase) {
    case application::ImportPhase::Idle:
        return QStringLiteral("等待导入");
    case application::ImportPhase::Inspecting:
        return QStringLiteral("读取整合包");
    case application::ImportPhase::PreparingGame:
        return QStringLiteral("准备游戏环境");
    case application::ImportPhase::InstallingFiles:
        return QStringLiteral("安装整合包文件");
    case application::ImportPhase::Committing:
        return QStringLiteral("完成安装");
    case application::ImportPhase::Succeeded:
        return QStringLiteral("安装完成");
    case application::ImportPhase::Cancelling:
        return QStringLiteral("正在取消");
    case application::ImportPhase::Cancelled:
        return QStringLiteral("已取消");
    case application::ImportPhase::Failed:
        return QStringLiteral("导入失败");
    }
    return QStringLiteral("状态未知");
}

QString importErrorText(application::ImportError error)
{
    switch (error) {
    case application::ImportError::InvalidArchive:
        return QStringLiteral("无法读取整合包，文件可能已损坏。");
    case application::ImportError::UnsupportedFormat:
        return QStringLiteral("暂不支持这个整合包格式。");
    case application::ImportError::UnsafePath:
        return QStringLiteral("整合包包含不安全的文件路径。");
    case application::ImportError::InvalidManifest:
        return QStringLiteral("整合包清单不完整或无法识别。");
    case application::ImportError::UnsupportedLoader:
        return QStringLiteral("暂不支持该整合包使用的加载器。");
    case application::ImportError::Network:
        return QStringLiteral("下载整合包文件时网络连接失败。");
    case application::ImportError::Integrity:
        return QStringLiteral("整合包文件校验失败。");
    case application::ImportError::Storage:
        return QStringLiteral("保存整合包文件失败，请检查磁盘空间和权限。");
    case application::ImportError::Conflict:
        return QStringLiteral("目标实例或文件与现有内容冲突。");
    case application::ImportError::JavaUnavailable:
        return QStringLiteral("没有找到可用的 Java 运行环境。");
    case application::ImportError::Cancelled:
        return QStringLiteral("导入已取消。");
    case application::ImportError::Busy:
        return QStringLiteral("当前有另一个任务正在进行。");
    }
    return QStringLiteral("导入未完成，请稍后重试。");
}

qint64 boundedQint64(std::uint64_t value)
{
    const auto limit = static_cast<std::uint64_t>(std::numeric_limits<qint64>::max());
    return static_cast<qint64>(std::min(value, limit));
}

} // namespace

HomePage::HomePage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("下午好"), QStringLiteral("正在读取当前状态"), QString());
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

    m_modpackSection = new QWidget(m_left);
    m_modpackSection->setObjectName(QStringLiteral("ModpackSection"));
    auto* modpackLayout = new QVBoxLayout(m_modpackSection);
    modpackLayout->setContentsMargins(0, 0, 0, 8);
    modpackLayout->setSpacing(6);
    auto* modpackTitle = new QLabel(QStringLiteral("整合包"), m_modpackSection);
    modpackTitle->setObjectName(QStringLiteral("SectionTitle"));
    modpackLayout->addWidget(modpackTitle);

    m_modpackDropZone = new QFrame(m_modpackSection);
    m_modpackDropZone->setObjectName(QStringLiteral("ImportDropZone"));
    m_modpackDropZone->setAcceptDrops(true);
    m_modpackDropZone->installEventFilter(this);
    auto* dropLayout = new QHBoxLayout(m_modpackDropZone);
    dropLayout->setContentsMargins(12, 8, 12, 8);
    dropLayout->setSpacing(10);
    auto* dropText = new QVBoxLayout();
    dropText->setContentsMargins(0, 0, 0, 0);
    dropText->setSpacing(2);
    auto* dropTitle = new QLabel(QStringLiteral("拖入整合包"), m_modpackDropZone);
    dropTitle->setObjectName(QStringLiteral("ImportDropTitle"));
    auto* dropSub = new QLabel(QStringLiteral("支持 .zip / .mrpack，一次一个"), m_modpackDropZone);
    dropSub->setObjectName(QStringLiteral("ImportDropSub"));
    dropSub->setWordWrap(true);
    dropText->addWidget(dropTitle);
    dropText->addWidget(dropSub);
    m_modpackPick = new QPushButton(QStringLiteral("选择整合包…"), m_modpackDropZone);
    m_modpackPick->setObjectName(QStringLiteral("DialogAction"));
    m_modpackPick->setCursor(Qt::PointingHandCursor);
    m_modpackPick->setAutoDefault(false);
    m_modpackPick->setAccessibleName(QStringLiteral("选择整合包文件"));
    dropLayout->addLayout(dropText, 1);
    dropLayout->addWidget(m_modpackPick, 0, Qt::AlignVCenter);
    modpackLayout->addWidget(m_modpackDropZone);

    auto* instanceRow = new QHBoxLayout();
    instanceRow->setContentsMargins(0, 2, 0, 0);
    instanceRow->setSpacing(8);
    auto* instanceLabel = new QLabel(QStringLiteral("当前实例"), m_modpackSection);
    instanceLabel->setObjectName(QStringLiteral("DetailLabel"));
    m_modpackInstance = new QComboBox(m_modpackSection);
    m_modpackInstance->setObjectName(QStringLiteral("ModpackInstanceCombo"));
    m_modpackInstance->setAccessibleName(QStringLiteral("当前游戏实例"));
    instanceRow->addWidget(instanceLabel, 0, Qt::AlignVCenter);
    instanceRow->addWidget(m_modpackInstance, 1);
    modpackLayout->addLayout(instanceRow);

    m_modpackPackName = new QLabel(m_modpackSection);
    m_modpackPackName->setObjectName(QStringLiteral("ModpackPackName"));
    m_modpackPackName->setTextFormat(Qt::PlainText);
    m_modpackPackName->setWordWrap(true);
    modpackLayout->addWidget(m_modpackPackName);
    m_modpackPhase = new QLabel(m_modpackSection);
    m_modpackPhase->setObjectName(QStringLiteral("ModpackPhase"));
    modpackLayout->addWidget(m_modpackPhase);

    m_modpackProgress = new QProgressBar(m_modpackSection);
    m_modpackProgress->setObjectName(QStringLiteral("ImportProgress"));
    m_modpackProgress->setTextVisible(false);
    m_modpackProgress->setRange(0, 1000);
    m_modpackProgress->hide();
    modpackLayout->addWidget(m_modpackProgress);
    m_modpackProgressText = new QLabel(m_modpackSection);
    m_modpackProgressText->setObjectName(QStringLiteral("ModpackProgressText"));
    m_modpackProgressText->setWordWrap(true);
    m_modpackProgressText->hide();
    modpackLayout->addWidget(m_modpackProgressText);

    m_modpackError = new QLabel(m_modpackSection);
    m_modpackError->setObjectName(QStringLiteral("ModpackError"));
    m_modpackError->setTextFormat(Qt::PlainText);
    m_modpackError->setWordWrap(true);
    m_modpackError->hide();
    modpackLayout->addWidget(m_modpackError);
    m_modpackHint = new QLabel(m_modpackSection);
    m_modpackHint->setObjectName(QStringLiteral("ModpackHint"));
    m_modpackHint->setTextFormat(Qt::PlainText);
    m_modpackHint->setWordWrap(true);
    m_modpackHint->hide();
    modpackLayout->addWidget(m_modpackHint);

    auto* importActions = new QHBoxLayout();
    importActions->setContentsMargins(0, 0, 0, 0);
    importActions->setSpacing(8);
    m_modpackCancel = new QPushButton(QStringLiteral("取消导入"), m_modpackSection);
    m_modpackCancel->setObjectName(QStringLiteral("CancelAction"));
    m_modpackCancel->setCursor(Qt::PointingHandCursor);
    m_modpackCancel->setAutoDefault(false);
    m_modpackCancel->setAccessibleName(QStringLiteral("取消导入"));
    m_modpackCancel->hide();
    m_modpackRetry = new QPushButton(QStringLiteral("重试导入"), m_modpackSection);
    m_modpackRetry->setObjectName(QStringLiteral("DialogAction"));
    m_modpackRetry->setCursor(Qt::PointingHandCursor);
    m_modpackRetry->setAutoDefault(false);
    m_modpackRetry->setAccessibleName(QStringLiteral("重试导入"));
    m_modpackRetry->hide();
    importActions->addWidget(m_modpackCancel);
    importActions->addWidget(m_modpackRetry);
    importActions->addStretch(1);
    modpackLayout->addLayout(importActions);

    m_modpackNote = new QLabel(
        QStringLiteral("可选客户端内容默认随包安装；安装完成后请手动启动游戏。"),
        m_modpackSection);
    m_modpackNote->setObjectName(QStringLiteral("Footnote"));
    m_modpackNote->setWordWrap(true);
    modpackLayout->addWidget(m_modpackNote);
    m_modpackSection->hide();
    const int summaryIndex = leftLayout->indexOf(summaryTitle);
    if (summaryIndex >= 0) {
        leftLayout->insertWidget(summaryIndex, m_modpackSection);
    } else {
        leftLayout->addWidget(m_modpackSection);
    }

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
    m_artwork->setObjectName(QStringLiteral("CampusArtwork"));
    m_artwork->setFixedSize(226, 280);
    rightLayout->addWidget(m_artwork, 0, Qt::AlignHCenter);
#ifdef HUSTMCL_PUBLIC_RELEASE
    m_artCaption = new QLabel(QStringLiteral("校园的树影落在桌面上，准备好就开始新的旅程。"), m_right);
#else
    m_artCaption = new QLabel(QStringLiteral("小科陪你，开启新世界。"), m_right);
#endif
    m_artCaption->setObjectName(QStringLiteral("SidebarCaption"));
    m_artCaption->setWordWrap(true);
#ifdef HUSTMCL_PUBLIC_RELEASE
    m_artSub = new QLabel(QStringLiteral("整理好行囊，从这一页出发。"), m_right);
#else
    m_artSub = new QLabel(QStringLiteral("树荫下的片刻，也是冒险的起点。"), m_right);
#endif
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
    m_content->setAcceptDrops(true);
    m_content->installEventFilter(this);
    m_left->setAcceptDrops(true);
    m_left->installEventFilter(this);
    m_right->setAcceptDrops(true);
    m_right->installEventFilter(this);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, &HomePage::primaryActionRequested);
    connect(m_actionBar, &PrimaryActionBar::secondaryClicked, this, [this]() {
        if (m_snapshot.offlineMode && m_task.cancelAvailable) {
            emit cancelRequested(m_task.taskId);
            return;
        }
        if (!m_snapshot.offlineMode) {
            emit installRequested();
        }
    });
    connect(m_settingsLink, &QPushButton::clicked, this, &HomePage::settingsRequested);
    connect(m_logsLink, &QPushButton::clicked, this, &HomePage::logsRequested);
    connect(m_modpackPick, &QPushButton::clicked, this, &HomePage::chooseImportFile);
    connect(m_modpackInstance, &QComboBox::currentIndexChanged,
            this, [this](int index) {
        if (!m_modpackAvailable || importLocked() || index < 0) {
            return;
        }
        const QString id = m_modpackInstance->itemData(index).toString();
        if (!id.isEmpty()) {
            emit modpackInstanceSelected(id);
        }
    });
    connect(m_modpackCancel, &QPushButton::clicked, this, [this]() {
        const auto phase = m_modpackSnapshot.phase;
        if (m_modpackAvailable
            && (phase == application::ImportPhase::Inspecting
                || phase == application::ImportPhase::PreparingGame
                || phase == application::ImportPhase::InstallingFiles)) {
            emit modpackCancelRequested(m_modpackSnapshot.taskId);
        }
    });
    connect(m_modpackRetry, &QPushButton::clicked, this, [this]() {
        if (m_modpackAvailable && !importLocked()) {
            emit modpackRetryRequested(m_modpackSnapshot.taskId);
        }
    });

    setAcceptDrops(true);
    setSnapshot({});
    updateResponsiveLayout();
}

void HomePage::setSnapshot(const application::HomeSnapshot& snapshot)
{
    m_snapshot = snapshot;
    m_primaryPending = false;

    const QString accountName = snapshot.account.displayName.isEmpty()
        ? (snapshot.offlineMode ? QStringLiteral("本地玩家") : QStringLiteral("尚未登录"))
        : snapshot.account.displayName;
    const QString greeting = snapshot.greeting.isEmpty()
        ? QStringLiteral("你好")
        : snapshot.greeting;
    const QString instanceName = snapshot.instance.displayName.isEmpty()
        ? QStringLiteral("尚未安装实例")
        : snapshot.instance.displayName;
    QString instanceMeta = snapshot.instance.versionLabel;
    if (!snapshot.instance.editionLabel.isEmpty()) {
        if (!instanceMeta.isEmpty()) {
            instanceMeta += QStringLiteral(" · ");
        }
        instanceMeta += snapshot.instance.editionLabel;
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
    const bool accountReady = snapshot.account.state == application::AccountState::SignedIn
        || snapshot.account.state == application::AccountState::Offline;
    m_accountRow->setData(snapshot.offlineMode ? QStringLiteral("离线身份") : QStringLiteral("账号"),
                          accountValue,
                          copy::accountStateText(snapshot.account.state),
                          accountReady);

    QString javaValue = snapshot.environment.versionLabel;
    if (!snapshot.environment.architectureLabel.isEmpty()) {
        javaValue += QStringLiteral(" · ") + snapshot.environment.architectureLabel;
    }
    if (javaValue.isEmpty()) {
        javaValue = QStringLiteral("自动选择");
    }
    m_javaRow->setData(QStringLiteral("Java 运行时"), javaValue,
                       copy::javaStateText(snapshot.environment.state),
                       snapshot.environment.state == application::JavaState::Ready);

    QString instanceValue = snapshot.instance.versionLabel;
    if (!snapshot.instance.editionLabel.isEmpty()) {
        if (!instanceValue.isEmpty()) {
            instanceValue += QStringLiteral(" · ");
        }
        instanceValue += snapshot.instance.editionLabel;
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
    if (snapshot.offlineMode) {
        m_footnote->setText(
            QStringLiteral("首次点击启动才会完整校验文件与 Java；游戏运行时可关闭窗口，游戏会继续运行。"));
    } else {
        m_footnote->setText(snapshot.launch.closeLauncherAfterLaunch
                                ? QStringLiteral("启动后可以关闭启动器，游戏会继续运行。")
                                : QStringLiteral("启动后启动器会保持打开。"));
    }

    applyActionBar();
    updateModpackUi();
}

void HomePage::setModpackAvailable(bool available)
{
    if (m_modpackAvailable == available) {
        updateModpackUi();
        return;
    }
    m_modpackAvailable = available;
    if (!available) {
        m_modpackSnapshot = {};
        if (m_modpackHint) {
            m_modpackHint->clear();
            m_modpackHint->hide();
        }
    }
    updateModpackUi();
}

void HomePage::setModpackSnapshot(const application::ModpackSnapshot& snapshot)
{
    const bool wasActive = application::importActive(m_modpackSnapshot.phase);
    m_modpackSnapshot = snapshot;
    if (snapshot.phase == application::ImportPhase::Idle && snapshot.taskId.isEmpty()) {
        m_modpackHint->clear();
        m_modpackHint->hide();
    }
    updateModpackUi();
    const bool nowActive = application::importActive(snapshot.phase);
    if (m_modpackAvailable && wasActive != nowActive && scrollArea()) {
        if (nowActive && m_modpackCancel->isVisible()) {
            scrollArea()->ensureWidgetVisible(m_modpackCancel);
        } else if (!nowActive && m_modpackRetry->isVisible()) {
            scrollArea()->ensureWidgetVisible(m_modpackRetry);
        } else if (m_modpackPhase->isVisible()) {
            scrollArea()->ensureWidgetVisible(m_modpackPhase);
        } else {
            scrollArea()->ensureWidgetVisible(m_modpackSection);
        }
    }
}

void HomePage::updateModpackUi()
{
    if (!m_modpackSection) {
        return;
    }
    m_modpackSection->setVisible(m_modpackAvailable);
    if (!m_modpackAvailable) {
        return;
    }

    const auto phase = m_modpackSnapshot.phase;
    const bool active = application::importActive(phase);
    const bool launchBusy = m_modpackSnapshot.launchActive || m_snapshot.sessionActive;
    const bool locked = active || launchBusy;
    m_modpackDropZone->setVisible(!active);

    {
        const QSignalBlocker blocker(m_modpackInstance);
        m_modpackInstance->clear();
        for (const application::InstalledInstance& instance : m_modpackSnapshot.instances) {
            QString label = instance.name.isEmpty() ? instance.id : instance.name;
            if (!instance.versionLabel.isEmpty()
                && !label.contains(instance.versionLabel, Qt::CaseInsensitive)) {
                label += QStringLiteral(" · ") + instance.versionLabel;
            }
            m_modpackInstance->addItem(label, instance.id);
            m_modpackInstance->setItemData(
                m_modpackInstance->count() - 1,
                QStringLiteral("版本 %1").arg(instance.versionLabel.isEmpty()
                                                  ? QStringLiteral("未知")
                                                  : instance.versionLabel),
                Qt::ToolTipRole);
        }
        const int selected = m_modpackInstance->findData(m_modpackSnapshot.selectedId);
        if (m_modpackInstance->count() == 0) {
            m_modpackInstance->addItem(QStringLiteral("暂无可用实例"), QString());
            m_modpackInstance->setCurrentIndex(0);
        } else {
            m_modpackInstance->setCurrentIndex(selected);
        }
    }
    m_modpackInstance->setEnabled(!locked && !m_modpackSnapshot.instances.empty());

    m_modpackPick->setEnabled(!locked);
    if (active) {
        m_modpackPick->setText(QStringLiteral("正在导入…"));
    } else if (launchBusy) {
        m_modpackPick->setText(QStringLiteral("游戏运行中"));
    } else {
        m_modpackPick->setText(QStringLiteral("选择整合包…"));
    }
    m_modpackDropZone->setToolTip(
        locked ? QStringLiteral("正在导入或游戏运行中，暂时不能导入新的整合包。")
               : QStringLiteral("拖入一个本地 .zip 或 .mrpack 文件。"));

    QString packName = m_modpackSnapshot.packName;
    if (packName.isEmpty() && !m_modpackSnapshot.archivePath.isEmpty()) {
        packName = QFileInfo(m_modpackSnapshot.archivePath).fileName();
    }
    if (packName.isEmpty()) {
        packName = phase == application::ImportPhase::Idle
            ? QStringLiteral("尚未选择整合包")
            : QStringLiteral("正在识别整合包…");
    }
    m_modpackPackName->setText(packName);
    m_modpackPhase->setText(importPhaseText(phase));

    const qint64 completedFiles = boundedQint64(m_modpackSnapshot.progress.completedFiles);
    const qint64 totalFiles = boundedQint64(m_modpackSnapshot.progress.totalFiles);
    const qint64 completedBytes = boundedQint64(m_modpackSnapshot.progress.completedBytes);
    const qint64 totalBytes = boundedQint64(m_modpackSnapshot.progress.totalBytes);
    const bool hasFileTotal = totalFiles > 0;
    const bool hasByteTotal = totalBytes > 0;
    const bool showBar = hasFileTotal || hasByteTotal;
    m_modpackProgress->setVisible(showBar);
    if (showBar) {
        int value = 0;
        if (hasByteTotal) {
            value = qRound(copy::progressFraction(completedBytes, totalBytes) * 1000.0);
        } else {
            const double fraction = std::min(1.0, static_cast<double>(completedFiles)
                                                      / static_cast<double>(totalFiles));
            value = qRound(fraction * 1000.0);
        }
        m_modpackProgress->setValue(value);
    }

    QStringList progressParts;
    if (hasFileTotal) {
        progressParts.append(QStringLiteral("文件 %1 / %2").arg(completedFiles).arg(totalFiles));
    } else if (completedFiles > 0) {
        progressParts.append(QStringLiteral("已处理 %1 个文件").arg(completedFiles));
    }
    if (hasByteTotal) {
        progressParts.append(QStringLiteral("已传输 %1 / %2（%3）")
                                 .arg(copy::formatBytes(completedBytes),
                                      copy::formatBytes(totalBytes),
                                      copy::formatPercent(completedBytes, totalBytes)));
    } else if (completedBytes > 0) {
        progressParts.append(QStringLiteral("已传输 %1 · 总量未知")
                                 .arg(copy::formatBytes(completedBytes)));
    }
    if (progressParts.isEmpty()
        && (phase == application::ImportPhase::PreparingGame
            || phase == application::ImportPhase::InstallingFiles
            || phase == application::ImportPhase::Committing)) {
        progressParts.append(QStringLiteral("总量未知"));
    }
    m_modpackProgressText->setText(progressParts.join(QStringLiteral(" · ")));
    m_modpackProgressText->setVisible(!progressParts.isEmpty());

    QString errorText;
    if (phase == application::ImportPhase::Failed) {
        if (m_modpackSnapshot.error.has_value()) {
            const application::ImportFailure& failure = m_modpackSnapshot.error.value();
            errorText = importErrorText(failure.code);
            if (!failure.context.trimmed().isEmpty()) {
                errorText += QLatin1Char('\n') + failure.context;
            }
        } else {
            errorText = QStringLiteral("导入未完成，请稍后重试。");
        }
    }
    m_modpackError->setText(errorText);
    m_modpackError->setVisible(!errorText.isEmpty());

    const bool cancelable = phase == application::ImportPhase::Inspecting
        || phase == application::ImportPhase::PreparingGame
        || phase == application::ImportPhase::InstallingFiles;
    if (cancelable) {
        m_modpackCancel->setText(QStringLiteral("取消导入"));
        m_modpackCancel->setEnabled(!launchBusy);
        m_modpackCancel->setToolTip(
            launchBusy ? QStringLiteral("游戏运行中，暂时不能取消导入。")
                       : QStringLiteral("停止当前导入，等待任务确认取消。"));
        m_modpackCancel->show();
    } else if (phase == application::ImportPhase::Committing) {
        m_modpackCancel->setText(QStringLiteral("正在完成安装"));
        m_modpackCancel->setEnabled(false);
        m_modpackCancel->setToolTip(QStringLiteral("正在完成安装，这个阶段不能取消。"));
        m_modpackCancel->show();
    } else if (phase == application::ImportPhase::Cancelling) {
        m_modpackCancel->setText(QStringLiteral("正在取消…"));
        m_modpackCancel->setEnabled(false);
        m_modpackCancel->setToolTip(QStringLiteral("正在等待当前安装停止并确认取消结果。"));
        m_modpackCancel->show();
    } else {
        m_modpackCancel->hide();
    }

    const bool retryable = phase == application::ImportPhase::Failed
        || phase == application::ImportPhase::Cancelled;
    m_modpackRetry->setVisible(retryable);
    m_modpackRetry->setEnabled(retryable && !launchBusy);
    m_modpackRetry->setToolTip(
        launchBusy ? QStringLiteral("游戏运行中，暂时不能重试导入。")
                   : QStringLiteral("重新读取整合包并导入。"));

    m_modpackHint->setVisible(m_modpackAvailable && !m_modpackHint->text().isEmpty());
}

bool HomePage::importLocked() const
{
    return application::importActive(m_modpackSnapshot.phase)
        || m_modpackSnapshot.launchActive
        || m_snapshot.sessionActive;
}

void HomePage::chooseImportFile()
{
    if (!m_modpackAvailable || importLocked()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择整合包"), QString(),
        QStringLiteral("整合包 (*.zip *.mrpack)"));
    if (!path.isEmpty()) {
        requestImport(path);
    }
}

void HomePage::requestImport(const QString& localPath)
{
    if (!m_modpackAvailable || importLocked() || localPath.isEmpty()) {
        return;
    }
    showImportHint(QString());
    emit modpackImportRequested(localPath);
}

void HomePage::showImportHint(const QString& text)
{
    if (!m_modpackHint) {
        return;
    }
    m_modpackHint->setText(text);
    const bool visible = m_modpackAvailable && !text.isEmpty();
    m_modpackHint->setVisible(visible);
    if (visible && scrollArea()) {
        scrollArea()->ensureWidgetVisible(m_modpackHint);
    }
}

void HomePage::dragEnterEvent(QDragEnterEvent* event)
{
    if (m_modpackAvailable && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void HomePage::dragMoveEvent(QDragMoveEvent* event)
{
    if (m_modpackAvailable && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void HomePage::dropEvent(QDropEvent* event)
{
    if (!m_modpackAvailable) {
        event->ignore();
        return;
    }
    const QMimeData* mime = event->mimeData();
    handleDroppedUrls(mime ? mime->urls() : QList<QUrl>());
    event->acceptProposedAction();
}

void HomePage::handleDroppedUrls(const QList<QUrl>& urls)
{
    if (!m_modpackAvailable) {
        return;
    }
    if (urls.isEmpty()) {
        showImportHint(QStringLiteral("请拖入本地 .zip 或 .mrpack 文件。"));
        return;
    }
    if (importLocked()) {
        showImportHint(QStringLiteral("正在导入或游戏运行中，暂时不能导入新的整合包。"));
        return;
    }
    if (urls.size() != 1) {
        showImportHint(QStringLiteral("一次只能导入一个整合包文件。"));
        return;
    }
    const QUrl url = urls.front();
    if (!url.isLocalFile()) {
        showImportHint(QStringLiteral("远程链接不能直接导入，请先下载到本地。"));
        return;
    }
    const QString path = url.toLocalFile();
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (path.isEmpty() || (suffix != QStringLiteral("zip") && suffix != QStringLiteral("mrpack"))) {
        showImportHint(QStringLiteral("请选择 .zip 或 .mrpack 整合包。"));
        return;
    }
    requestImport(path);
}

bool HomePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_modpackDropZone || watched == m_content || watched == m_left
        || watched == m_right) {
        switch (event->type()) {
        case QEvent::DragEnter:
            dragEnterEvent(static_cast<QDragEnterEvent*>(event));
            return true;
        case QEvent::DragMove:
            dragMoveEvent(static_cast<QDragMoveEvent*>(event));
            return true;
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent*>(event));
            return true;
        default:
            break;
        }
    }
    return PageScaffold::eventFilter(watched, event);
}

void HomePage::setTaskSnapshot(const application::TaskSnapshot& task)
{
    m_task = task;
    applyActionBar();
}

void HomePage::setPrimaryRequestPending(bool pending)
{
    if (m_primaryPending == pending) {
        return;
    }
    m_primaryPending = pending;
    applyActionBar();
}

void HomePage::applyActionBar()
{
    const application::HomeSnapshot& snapshot = m_snapshot;
    const QString primaryText = snapshot.primaryLabel.isEmpty()
        ? copy::primaryActionText(snapshot.primaryAction)
        : snapshot.primaryLabel;
    m_actionBar->setTitle(actionTitle(snapshot));
    m_actionBar->setNote(snapshot.primaryReason.isEmpty()
                             ? QStringLiteral("正在读取当前状态…")
                             : snapshot.primaryReason);
    const bool primaryEnabled = snapshot.primaryEnabled && !m_primaryPending;
    QString primaryTip = snapshot.primaryReason;
    if (m_primaryPending) {
        primaryTip = QStringLiteral("已发送启动请求，请稍候。");
    }
    m_actionBar->setPrimary(primaryText, primaryEnabled, primaryTip);
    m_actionBar->setPrimaryVisible(!primaryText.isEmpty());

    if (snapshot.offlineMode) {
        if (m_task.cancelAvailable) {
            m_actionBar->setSecondary(
                QStringLiteral("取消准备"), true, true,
                QStringLiteral("取消当前启动准备，不会删除游戏文件或存档。"), true);
        } else {
            m_actionBar->setSecondary(
                QStringLiteral("安装其他版本（未接入）"), true, false,
                QStringLiteral("当前仅支持启动已安装的本地 26.2 离线实例；下载其他版本尚未实现。"),
                false);
        }
    } else {
        m_actionBar->setSecondary(QStringLiteral("安装其他版本"), true, true,
                                  QStringLiteral("打开安装页"));
    }
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
    if (!snapshot.statusTitle.isEmpty()) {
        return snapshot.statusTitle;
    }
    if (snapshot.primaryAction == application::PrimaryAction::None) {
        return QStringLiteral("暂无可用操作");
    }
    if (snapshot.offlineMode) {
        if (snapshot.sessionActive) {
            return QStringLiteral("游戏运行中");
        }
        if (snapshot.primaryAction == application::PrimaryAction::Launch) {
            return QStringLiteral("准备就绪");
        }
        return QStringLiteral("本地离线");
    }
    if (snapshot.primaryAction == application::PrimaryAction::Launch) {
        return QStringLiteral("准备就绪");
    }
    return QStringLiteral("下一步");
}

} // namespace hustmcl::ui
