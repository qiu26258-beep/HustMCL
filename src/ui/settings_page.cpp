#include "settings_page.h"

#include "combo_box.h"
#include "copy.h"
#include "elided_label.h"
#include "hustmcl/application/launcher_settings.h"
#include "page_header.h"
#include "primary_action_bar.h"
#include "summary_row.h"
#include "voxel_icons.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

namespace hustmcl::ui {

namespace {
constexpr int kPresetValuesMb[] = {2048, 4096, 6144, 8192};
}

SettingsPage::SettingsPage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("设置"),
                         QStringLiteral("主题外观、账号身份与运行配置"),
                         QString());

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(14);

    // ==================== CARD 1: THEME & TIME AUTO-SWITCH ====================
    auto* themeCard = new QFrame(content);
    themeCard->setObjectName(QStringLiteral("VoxelCard"));
    auto* themeLayout = new QVBoxLayout(themeCard);
    themeLayout->setContentsMargins(14, 12, 14, 12);
    themeLayout->setSpacing(6);

    auto* themeTitle = new QLabel(QStringLiteral("像素主题与昼夜切换"), themeCard);
    themeTitle->setObjectName(QStringLiteral("SectionTitle"));
    themeLayout->addWidget(themeTitle);

    auto* themeRow = new QHBoxLayout();
    themeRow->setContentsMargins(0, 2, 0, 0);
    themeRow->setSpacing(10);
    m_themeMode = new ComboBox(themeCard);
    m_themeMode->setObjectName(QStringLiteral("ThemeModeSelector"));
    m_themeMode->setAccessibleName(QStringLiteral("主题模式"));
    m_themeMode->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_themeMode->setMinimumContentsLength(14);
    m_themeMode->addItem(QStringLiteral("随本地时间自动切换（06:00 晴昼 / 18:00 深林）"),
                         static_cast<int>(theme::ThemeMode::AutoByTime));
    m_themeMode->addItem(QStringLiteral("喻园深林（深色体素）"),
                         static_cast<int>(theme::ThemeMode::Dark));
    m_themeMode->addItem(QStringLiteral("梧桐晴昼（浅色砂岩）"),
                         static_cast<int>(theme::ThemeMode::Light));
    themeRow->addWidget(m_themeMode, 1);
    themeLayout->addLayout(themeRow);

    m_themeHint = new QLabel(themeCard);
    m_themeHint->setObjectName(QStringLiteral("DetailNote"));
    m_themeHint->setWordWrap(true);
    themeLayout->addWidget(m_themeHint);
    layout->addWidget(themeCard);

    // ==================== CARD 2: IDENTITY & LOGIN ====================
    auto* identityCard = new QFrame(content);
    identityCard->setObjectName(QStringLiteral("VoxelCard"));
    auto* identityCardLayout = new QVBoxLayout(identityCard);
    identityCardLayout->setContentsMargins(14, 12, 14, 12);
    identityCardLayout->setSpacing(8);

    auto* modeTitle = new QLabel(QStringLiteral("登录方式"), identityCard);
    modeTitle->setObjectName(QStringLiteral("SectionTitle"));
    identityCardLayout->addWidget(modeTitle);

    m_loginMode = new ComboBox(identityCard);
    m_loginMode->setObjectName(QStringLiteral("LoginModeSelector"));
    m_loginMode->setAccessibleName(QStringLiteral("登录方式"));
    m_loginMode->addItem(QStringLiteral("微软账号（正版）"));
    m_loginMode->addItem(QStringLiteral("离线玩家（本地）"));
    identityCardLayout->addWidget(m_loginMode);

    m_modeError = new QLabel(identityCard);
    m_modeError->setObjectName(QStringLiteral("LoginModeError"));
    m_modeError->setWordWrap(true);
    m_modeError->hide();
    identityCardLayout->addWidget(m_modeError);

    m_offlineSection = new QWidget(identityCard);
    m_offlineSection->setObjectName(QStringLiteral("OfflineIdentitySection"));
    auto* offlineLayout = new QVBoxLayout(m_offlineSection);
    offlineLayout->setContentsMargins(0, 4, 0, 0);
    offlineLayout->setSpacing(6);

    auto* identityTitle = new QLabel(QStringLiteral("离线玩家"), m_offlineSection);
    identityTitle->setObjectName(QStringLiteral("SectionTitle"));
    offlineLayout->addWidget(identityTitle);
    m_identityNote = new QLabel(
        QStringLiteral("离线玩家名仅用于本地单机游玩，不是微软正版登录。"
                       "改名会改变离线 UUID，也会影响已有存档中的角色背包与数据归属。"),
        m_offlineSection);
    m_identityNote->setObjectName(QStringLiteral("DetailNote"));
    m_identityNote->setWordWrap(true);
    offlineLayout->addWidget(m_identityNote);

    auto* fieldLabel = new QLabel(QStringLiteral("离线玩家名"), m_offlineSection);
    fieldLabel->setObjectName(QStringLiteral("DetailLabel"));
    offlineLayout->addWidget(fieldLabel);
    m_nameInput = new QLineEdit(m_offlineSection);
    m_nameInput->setObjectName(QStringLiteral("PlayerNameInput"));
    m_nameInput->setPlaceholderText(QStringLiteral("例如 HustPlayer"));
    m_nameInput->setMaxLength(64);
    m_nameInput->setClearButtonEnabled(true);
    m_nameInput->setAccessibleName(QStringLiteral("离线玩家名"));
    m_nameInput->setToolTip(
        QStringLiteral("仅支持 1–16 位英文字母、数字或下划线。"));
    offlineLayout->addWidget(m_nameInput);
    m_fieldError = new QLabel(m_offlineSection);
    m_fieldError->setObjectName(QStringLiteral("FieldError"));
    m_fieldError->setWordWrap(true);
    m_fieldError->hide();
    offlineLayout->addWidget(m_fieldError);
    m_savedNote = new QLabel(m_offlineSection);
    m_savedNote->setObjectName(QStringLiteral("FieldSuccess"));
    m_savedNote->setWordWrap(true);
    m_savedNote->hide();
    offlineLayout->addWidget(m_savedNote);
    identityCardLayout->addWidget(m_offlineSection);

    m_accountSection = new QWidget(identityCard);
    m_accountSection->setObjectName(QStringLiteral("AccountPanel"));
    auto* accountLayout = new QVBoxLayout(m_accountSection);
    accountLayout->setContentsMargins(0, 4, 0, 0);
    accountLayout->setSpacing(6);

    auto* accountTitle = new QLabel(QStringLiteral("微软账号"), m_accountSection);
    accountTitle->setObjectName(QStringLiteral("SectionTitle"));
    accountLayout->addWidget(accountTitle);
    m_accountNote = new QLabel(
        QStringLiteral("使用自己的微软账号登录；登录成功只更新账号状态，游戏仍需手动启动。"),
        m_accountSection);
    m_accountNote->setObjectName(QStringLiteral("DetailNote"));
    m_accountNote->setWordWrap(true);
    accountLayout->addWidget(m_accountNote);

    auto* accountSlot = new QFrame(m_accountSection);
    accountSlot->setObjectName(QStringLiteral("VoxelSlot"));
    auto* slotRow = new QHBoxLayout(accountSlot);
    slotRow->setContentsMargins(10, 8, 10, 8);
    slotRow->setSpacing(10);

    m_accountAvatar = new QLabel(accountSlot);
    m_accountAvatar->setObjectName(QStringLiteral("Avatar"));
    m_accountAvatar->setAlignment(Qt::AlignCenter);
    m_accountAvatar->setPixmap(voxel::renderIcon(voxel::IconKind::PlayerHead, 30, devicePixelRatioF()));

    auto* accountMetaCol = new QVBoxLayout();
    accountMetaCol->setContentsMargins(0, 0, 0, 0);
    accountMetaCol->setSpacing(2);
    m_accountName = new QLabel(accountSlot);
    m_accountName->setObjectName(QStringLiteral("SettingsAccountName"));
    m_accountName->setTextFormat(Qt::PlainText);
    m_accountName->setWordWrap(true);
    m_accountMeta = new QLabel(accountSlot);
    m_accountMeta->setObjectName(QStringLiteral("SettingsAccountState"));
    m_accountMeta->setTextFormat(Qt::PlainText);
    m_accountMeta->setWordWrap(true);
    accountMetaCol->addWidget(m_accountName);
    accountMetaCol->addWidget(m_accountMeta);
    slotRow->addWidget(m_accountAvatar, 0, Qt::AlignVCenter);
    slotRow->addLayout(accountMetaCol, 1);
    accountLayout->addWidget(accountSlot);

    m_accountMessage = new QLabel(m_accountSection);
    m_accountMessage->setObjectName(QStringLiteral("AccountMessage"));
    m_accountMessage->setTextFormat(Qt::PlainText);
    m_accountMessage->setWordWrap(true);
    m_accountMessage->hide();
    accountLayout->addWidget(m_accountMessage);
    m_accountHint = new QLabel(m_accountSection);
    m_accountHint->setObjectName(QStringLiteral("DetailNote"));
    m_accountHint->setWordWrap(true);
    accountLayout->addWidget(m_accountHint);

    auto* accountButtons = new QHBoxLayout();
    accountButtons->setContentsMargins(0, 2, 0, 0);
    accountButtons->setSpacing(8);
    m_signInButton = new QPushButton(QStringLiteral("登录微软账号"), m_accountSection);
    m_signInButton->setObjectName(QStringLiteral("AccountSignIn"));
    m_signInButton->setCursor(Qt::PointingHandCursor);
    m_signInButton->setAutoDefault(false);
    m_signInButton->setAccessibleName(QStringLiteral("登录微软账号"));
    m_cancelSignInButton = new QPushButton(QStringLiteral("取消登录"), m_accountSection);
    m_cancelSignInButton->setObjectName(QStringLiteral("AccountCancelSignIn"));
    m_cancelSignInButton->setCursor(Qt::PointingHandCursor);
    m_cancelSignInButton->setAutoDefault(false);
    m_cancelSignInButton->setAccessibleName(QStringLiteral("取消登录"));
    m_signOutButton = new QPushButton(QStringLiteral("退出账号"), m_accountSection);
    m_signOutButton->setObjectName(QStringLiteral("AccountSignOut"));
    m_signOutButton->setCursor(Qt::PointingHandCursor);
    m_signOutButton->setAutoDefault(false);
    m_signOutButton->setAccessibleName(QStringLiteral("退出账号"));
    accountButtons->addWidget(m_signInButton);
    accountButtons->addWidget(m_cancelSignInButton);
    accountButtons->addWidget(m_signOutButton);
    accountButtons->addStretch(1);
    accountLayout->addLayout(accountButtons);
    identityCardLayout->addWidget(m_accountSection);
    layout->addWidget(identityCard);

    // ==================== CARD 3: RUNTIME & JVM MEMORY CONTROL ====================
    auto* runtimeCard = new QFrame(content);
    runtimeCard->setObjectName(QStringLiteral("VoxelCard"));
    auto* runtimeLayout = new QVBoxLayout(runtimeCard);
    runtimeLayout->setContentsMargins(14, 12, 14, 12);
    runtimeLayout->setSpacing(8);

    auto* runtimeTitle = new QLabel(QStringLiteral("运行配置与内存分配"), runtimeCard);
    runtimeTitle->setObjectName(QStringLiteral("SectionTitle"));
    runtimeLayout->addWidget(runtimeTitle);
    auto* runtimeNote = new QLabel(
        QStringLiteral("修改内存上限将持久化保存，并对下一次启动游戏生效；已在运行的游戏不受影响。"),
        runtimeCard);
    runtimeNote->setObjectName(QStringLiteral("DetailNote"));
    runtimeNote->setWordWrap(true);
    runtimeLayout->addWidget(runtimeNote);

    auto* memLabel = new QLabel(
        QStringLiteral("JVM 内存上限（%1–%2 MB）")
            .arg(application::kMinMemoryMb)
            .arg(application::kMaxMemoryMb),
        runtimeCard);
    memLabel->setObjectName(QStringLiteral("DetailLabel"));
    runtimeLayout->addWidget(memLabel);

    auto* memControlRow = new QHBoxLayout();
    memControlRow->setContentsMargins(0, 0, 0, 0);
    memControlRow->setSpacing(6);

    m_memoryInput = new QSpinBox(runtimeCard);
    m_memoryInput->setObjectName(QStringLiteral("MemoryInput"));
    m_memoryInput->setRange(application::kMinMemoryMb, application::kMaxMemoryMb);
    m_memoryInput->setSingleStep(256);
    m_memoryInput->setSuffix(QStringLiteral(" MB"));
    m_memoryInput->setValue(application::kDefaultMemoryMb);
    m_memoryInput->setMinimumWidth(130);
    m_memoryInput->setAccessibleName(QStringLiteral("内存上限"));
    m_memoryInput->setToolTip(
        QStringLiteral("输入或选择下次启动游戏的 JVM 最大内存（%1–%2 MB）。")
            .arg(application::kMinMemoryMb)
            .arg(application::kMaxMemoryMb));
    memControlRow->addWidget(m_memoryInput, 1);

    for (int presetMb : kPresetValuesMb) {
        auto* btn = new QPushButton(QStringLiteral("%1 GB").arg(presetMb / 1024), runtimeCard);
        btn->setObjectName(QStringLiteral("MemoryPreset"));
        btn->setProperty("memoryMb", presetMb);
        btn->setProperty("active", false);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoDefault(false);
        btn->setAccessibleName(QStringLiteral("设为 %1 GB").arg(presetMb / 1024));
        connect(btn, &QPushButton::clicked, this, [this, presetMb]() {
            m_memoryEdited = true;
            m_memoryInput->setValue(presetMb);
            clearFieldFeedback();
        });
        m_memoryPresets.append(btn);
        memControlRow->addWidget(btn, 0);
    }
    runtimeLayout->addLayout(memControlRow);

    m_memoryError = new QLabel(runtimeCard);
    m_memoryError->setObjectName(QStringLiteral("FieldError"));
    m_memoryError->setWordWrap(true);
    m_memoryError->hide();
    runtimeLayout->addWidget(m_memoryError);

    m_memorySaved = new QLabel(runtimeCard);
    m_memorySaved->setObjectName(QStringLiteral("FieldSuccess"));
    m_memorySaved->setWordWrap(true);
    m_memorySaved->hide();
    runtimeLayout->addWidget(m_memorySaved);

    auto* runtimeDiv = new QFrame(runtimeCard);
    runtimeDiv->setObjectName(QStringLiteral("Divider"));
    runtimeDiv->setFixedHeight(2);
    runtimeLayout->addWidget(runtimeDiv);

    m_versionRow = new SummaryRow(runtimeCard);
    m_javaRow = new SummaryRow(runtimeCard);
    runtimeLayout->addWidget(m_versionRow);
    runtimeLayout->addWidget(m_javaRow);

    auto* dataLabel = new QLabel(QStringLiteral("数据目录"), runtimeCard);
    dataLabel->setObjectName(QStringLiteral("DetailLabel"));
    runtimeLayout->addWidget(dataLabel);
    m_dataPath = new ElidedLabel(runtimeCard);
    m_dataPath->setObjectName(QStringLiteral("PathValue"));
    m_dataPath->setFixedHeight(34);
    runtimeLayout->addWidget(m_dataPath);
    layout->addWidget(runtimeCard);

    layout->addStretch(1);
    setContent(content);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, [this]() {
        clearFieldFeedback();
        if (m_offlineMode) {
            emit saveNameRequested(m_nameInput->text());
            emit saveSettingsRequested(m_nameInput->text(), m_memoryInput->value());
        } else {
            emit saveMemoryRequested(m_memoryInput->value());
        }
    });
    connect(m_nameInput, &QLineEdit::textEdited, this, [this]() {
        m_nameEdited = true;
        clearFieldFeedback();
    });
    connect(m_nameInput, &QLineEdit::returnPressed, this, [this]() {
        if (m_offlineMode && m_actionBar->primaryButton()->isEnabled()) {
            m_actionBar->primaryButton()->click();
        }
    });
    connect(m_memoryInput, &QSpinBox::valueChanged, this, [this](int value) {
        updatePresetHighlights(value);
        if (m_memoryInput->hasFocus()) {
            m_memoryEdited = true;
            clearFieldFeedback();
        }
    });
    connect(m_themeMode, &QComboBox::activated, this, [this](int index) {
        const auto mode = static_cast<theme::ThemeMode>(m_themeMode->itemData(index).toInt());
        emit themeModeRequested(mode);
    });
    connect(m_signInButton, &QPushButton::clicked, this, &SettingsPage::signInRequested);
    connect(m_cancelSignInButton, &QPushButton::clicked, this,
            &SettingsPage::cancelSignInRequested);
    connect(m_signOutButton, &QPushButton::clicked, this, &SettingsPage::signOutRequested);
    connect(m_loginMode, &QComboBox::activated, this, [this](int index) {
        emit offlineModeRequested(index == 1);
        // The gateway may reject a change; always display its confirmed mode.
        m_loginMode->setCurrentIndex(m_home.offlineMode ? 1 : 0);
    });

    setThemeState(theme::ThemeMode::AutoByTime, theme::activeTheme());
    setSnapshot({});
}

void SettingsPage::setThemeState(theme::ThemeMode mode, theme::ResolvedTheme resolved)
{
    const QSignalBlocker blocker(m_themeMode);
    const int idx = m_themeMode->findData(static_cast<int>(mode));
    if (idx >= 0) {
        m_themeMode->setCurrentIndex(idx);
    }
    if (mode == theme::ThemeMode::AutoByTime) {
        m_themeHint->setText(
            QStringLiteral("当前已按本地时间自动启用「%1」，并在日出/日落时段自动切换。")
                .arg(theme::resolvedThemeLabel(resolved)));
    } else {
        m_themeHint->setText(
            QStringLiteral("当前已锁定为「%1」，顶部标题栏也可随时一键切换。")
                .arg(theme::resolvedThemeLabel(resolved)));
    }
    if (m_accountAvatar) {
        m_accountAvatar->setPixmap(
            voxel::renderIcon(voxel::IconKind::PlayerHead, 30, devicePixelRatioF()));
    }
}

void SettingsPage::setSnapshot(const application::HomeSnapshot& home)
{
    if (m_offlineMode != home.offlineMode) {
        m_nameEdited = false;
        m_memoryEdited = false;
        clearFieldFeedback();
        m_modeError->hide();
    }
    m_home = home;
    m_sessionActive = home.sessionActive;
    m_offlineMode = home.offlineMode;

    const QSignalBlocker modeSignals(m_loginMode);
    m_loginMode->setCurrentIndex(m_offlineMode ? 1 : 0);
    m_loginMode->setEnabled(home.canChangeLoginMode);
    m_loginMode->setToolTip(home.canChangeLoginMode
        ? QStringLiteral("选择本次游戏使用的身份；登录方式会保存到本机。")
        : QStringLiteral("启动准备、游戏运行或登录进行中不能切换身份。"));

    if (m_offlineMode) {
        header()->setContent(QStringLiteral("设置"),
                             QStringLiteral("主题外观、离线玩家与游戏内存配置"),
                             QStringLiteral("离线模式"));
        const QString name = home.account.displayName;
        if (!m_nameEdited && m_nameInput->text() != name) {
            m_nameInput->setText(name);
        }
    } else {
        header()->setContent(QStringLiteral("设置"),
                             QStringLiteral("主题外观、微软账号与游戏内存配置"),
                             QStringLiteral("微软账号"));
    }

    const int snapshotMem = home.launch.memoryMb > 0
        ? home.launch.memoryMb
        : application::kDefaultMemoryMb;
    if (!m_memoryEdited) {
        const QSignalBlocker memBlocker(m_memoryInput);
        m_memoryInput->setValue(
            qBound(application::kMinMemoryMb, snapshotMem, application::kMaxMemoryMb));
        updatePresetHighlights(m_memoryInput->value());
    }

    m_versionRow->setData(QStringLiteral("游戏版本"),
                          home.instance.versionLabel.isEmpty()
                              ? QStringLiteral("未知")
                              : home.instance.versionLabel);
    QString javaSource = home.environment.sourceLabel;
    if (javaSource.isEmpty()) {
        javaSource = home.environment.versionLabel.isEmpty()
            ? QStringLiteral("启动时检测")
            : home.environment.versionLabel;
    }
    m_javaRow->setData(QStringLiteral("Java 来源"), javaSource);
    m_dataPath->setFullText(home.dataPath);
    m_dataPath->setToolTip(home.dataPath.isEmpty()
                               ? QStringLiteral("正在读取数据目录…")
                               : home.dataPath);

    applyAccountState();
    applyEnabledState();
}

void SettingsPage::setModeChangeResult(bool accepted)
{
    m_modeError->setText(
        accepted ? QString()
                 : QStringLiteral("无法保存登录方式，请检查数据目录的写入权限后重试。"));
    m_modeError->setVisible(!accepted);
}

void SettingsPage::setSaveResult(bool accepted, const QString& message)
{
    if (accepted) {
        m_nameEdited = false;
        m_fieldError->hide();
        m_savedNote->setText(message.isEmpty()
                                 ? QStringLiteral("离线名已应用，下次启动时生效。")
                                 : message);
        m_savedNote->show();
        m_actionBar->setErrorState(m_memoryError->isVisible());
        return;
    }
    m_savedNote->hide();
    m_fieldError->setText(message.isEmpty()
                              ? QStringLiteral("离线名格式不正确，请检查后重试。")
                              : message);
    m_fieldError->show();
    m_actionBar->setErrorState(true);
    if (m_offlineMode) {
        m_nameInput->setFocus(Qt::OtherFocusReason);
    }
}

void SettingsPage::setMemorySaveResult(bool accepted, const QString& message)
{
    if (accepted) {
        m_memoryEdited = false;
        m_memoryError->hide();
        m_memorySaved->setText(message.isEmpty()
                                   ? QStringLiteral("内存上限已保存，下次启动游戏时生效。")
                                   : message);
        m_memorySaved->show();
        m_actionBar->setErrorState(m_fieldError->isVisible());
        return;
    }
    m_memorySaved->hide();
    m_memoryError->setText(
        message.isEmpty()
            ? QStringLiteral("内存需在 %1–%2 MB 之间。")
                  .arg(application::kMinMemoryMb)
                  .arg(application::kMaxMemoryMb)
            : message);
    m_memoryError->show();
    m_actionBar->setErrorState(true);
}

void SettingsPage::focusNameField()
{
    if (m_offlineMode) {
        if (m_nameInput->isEnabled()) {
            m_nameInput->setFocus(Qt::OtherFocusReason);
            m_nameInput->selectAll();
            return;
        }
        if (m_actionBar->primaryButton()->isVisible()
            && m_actionBar->primaryButton()->isEnabled()) {
            m_actionBar->primaryButton()->setFocus(Qt::OtherFocusReason);
        }
        return;
    }
    for (QPushButton* button : {m_signInButton, m_cancelSignInButton, m_signOutButton}) {
        if (button->isVisible() && button->isEnabled()) {
            button->setFocus(Qt::OtherFocusReason);
            return;
        }
    }
}

void SettingsPage::updatePresetHighlights(int memoryMb)
{
    for (QPushButton* btn : m_memoryPresets) {
        const bool active = btn->property("memoryMb").toInt() == memoryMb;
        if (btn->property("active").toBool() != active) {
            btn->setProperty("active", active);
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
            btn->update();
        }
    }
}

void SettingsPage::applyEnabledState()
{
    m_offlineSection->setVisible(m_offlineMode);
    m_accountSection->setVisible(!m_offlineMode);

    const bool editable = !m_sessionActive;
    m_memoryInput->setEnabled(editable);
    for (QPushButton* btn : m_memoryPresets) {
        btn->setEnabled(editable);
    }

    if (m_offlineMode) {
        m_nameInput->setEnabled(editable);
        m_actionBar->setPrimary(QStringLiteral("保存设置"), editable,
                                editable ? QStringLiteral("保存离线玩家名与内存上限，下次启动游戏时生效。")
                                         : QStringLiteral("游戏运行中不能修改离线玩家名或内存设置。"));
        m_actionBar->setPrimaryVisible(true);
        m_actionBar->setSecondary(QString(), false, false, {});
        if (m_sessionActive) {
            m_actionBar->setTitle(QStringLiteral("游戏运行中"));
            m_actionBar->setNote(QStringLiteral("请先退出游戏，再修改离线玩家名或内存上限。"));
            m_identityNote->setText(
                QStringLiteral("游戏运行中不能修改离线玩家名。关闭启动器窗口不会结束游戏。"));
        } else {
            m_actionBar->setTitle(QStringLiteral("游戏设置"));
            m_actionBar->setNote(QStringLiteral("保存仅更新离线玩家名与内存上限，下次启动生效，不会自动进游戏。"));
            m_identityNote->setText(
                QStringLiteral("离线模式无需微软登录，适合单机游玩；无法加入要求正版验证的服务器或 Realms。"
                               "改名会改变离线 UUID，也会影响已有存档中的角色背包与数据归属。"));
        }
        return;
    }

    const application::AccountSnapshot& account = m_home.account;
    const bool busy = account.busy;
    const bool sessionActive = m_home.sessionActive;

    if (busy) {
        m_signInButton->setText(QStringLiteral("登录中…"));
    } else if (account.state == application::AccountState::SignedIn
               || account.state == application::AccountState::Expired
               || account.state == application::AccountState::Error) {
        m_signInButton->setText(QStringLiteral("重新登录"));
    } else {
        m_signInButton->setText(QStringLiteral("登录微软账号"));
    }
    m_signInButton->setAccessibleName(m_signInButton->text());

    const bool signInEnabled = account.canSignIn && !busy && !sessionActive;
    const bool signOutEnabled = account.canSignOut && !busy && !sessionActive;
    const bool cancelEnabled = busy && !sessionActive;
    m_signInButton->setEnabled(signInEnabled);
    m_signOutButton->setEnabled(signOutEnabled);
    m_cancelSignInButton->setEnabled(cancelEnabled);

    QString signInTip = QStringLiteral("使用微软账号登录。");
    if (sessionActive) {
        signInTip = QStringLiteral("游戏运行或启动准备中，不能登录。");
    } else if (busy) {
        signInTip = QStringLiteral("正在登录或续期，请稍候。");
    } else if (!account.canSignIn) {
        signInTip = QStringLiteral("当前不能登录，请稍后再试。");
    } else if (account.state == application::AccountState::SignedIn) {
        signInTip = QStringLiteral("重新登录会替换当前账号。");
    }
    m_signInButton->setToolTip(signInTip);
    m_signInButton->setAccessibleDescription(signInTip);

    QString signOutTip = QStringLiteral("退出并清除本应用保存的登录信息。");
    if (sessionActive) {
        signOutTip = QStringLiteral("游戏运行或启动准备中，不能退出账号。");
    } else if (busy) {
        signOutTip = QStringLiteral("登录进行中，暂时不能退出账号。");
    } else if (!account.canSignOut) {
        signOutTip = QStringLiteral("当前没有可退出的账号。");
    }
    m_signOutButton->setToolTip(signOutTip);
    m_signOutButton->setAccessibleDescription(signOutTip);

    const QString cancelTip = cancelEnabled
        ? QStringLiteral("取消本次登录等待。")
        : QStringLiteral("当前没有进行中的登录。");
    m_cancelSignInButton->setToolTip(cancelTip);
    m_cancelSignInButton->setAccessibleDescription(cancelTip);

    m_actionBar->setTitle(QStringLiteral("账号与内存配置"));
    m_actionBar->setNote(sessionActive
                             ? QStringLiteral("游戏运行中不能登录、退出账号或修改内存上限。")
                             : QStringLiteral("保存内存上限对下次启动生效；账号登录仍需手动启动游戏。"));
    m_actionBar->setPrimary(QStringLiteral("保存内存设置"), editable,
                            editable ? QStringLiteral("保存内存上限，下次启动游戏时生效。")
                                     : QStringLiteral("游戏运行中不能修改内存上限。"));
    m_actionBar->setPrimaryVisible(true);
    m_actionBar->setSecondary(QString(), false, false, {});
    m_actionBar->setErrorState(false);
}

void SettingsPage::applyAccountState()
{
    if (!m_accountSection || m_offlineMode) {
        return;
    }
    const application::AccountSnapshot& account = m_home.account;
    QString name = account.displayName.trimmed();
    if (name.isEmpty()) {
        name = QStringLiteral("尚未登录");
    }
    m_accountName->setText(name);

    QString meta = copy::accountStateText(account.state);
    if (!account.providerLabel.trimmed().isEmpty()) {
        meta += QStringLiteral(" · ") + account.providerLabel.trimmed();
    }
    m_accountMeta->setText(meta);

    m_accountMessage->setText(account.message);
    m_accountMessage->setVisible(!account.message.isEmpty());
    const bool error = account.state == application::AccountState::Error
        || account.state == application::AccountState::Expired;
    m_accountMessage->setProperty("error", error);
    m_accountMessage->style()->unpolish(m_accountMessage);
    m_accountMessage->style()->polish(m_accountMessage);

    QString hint;
    if (m_home.sessionActive) {
        hint = QStringLiteral("游戏运行中，账号操作暂不可用。");
    } else if (account.busy) {
        hint = QStringLiteral("请在弹出的浏览器中完成授权；也可以取消本次登录。");
    } else if (account.state == application::AccountState::SignedIn) {
        hint = QStringLiteral("已登录，可以随时重新登录或退出账号。");
    } else if (account.state == application::AccountState::Error
               || account.state == application::AccountState::Expired) {
        hint = QStringLiteral("重新登录后可继续使用正版账号。");
    } else if (account.remembered) {
        hint = QStringLiteral("上次登录信息已保存，可重新登录继续使用。");
    } else {
        hint = QStringLiteral("登录需要浏览器完成微软授权。");
    }
    m_accountHint->setText(hint);
}

void SettingsPage::clearFieldFeedback()
{
    m_fieldError->hide();
    m_savedNote->hide();
    m_memoryError->hide();
    m_memorySaved->hide();
    m_actionBar->setErrorState(false);
}

} // namespace hustmcl::ui
