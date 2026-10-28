#include "settings_page.h"

#include "copy.h"
#include "elided_label.h"
#include "page_header.h"
#include "primary_action_bar.h"
#include "summary_row.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace hustmcl::ui {

SettingsPage::SettingsPage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("设置"),
                         QStringLiteral("账号与运行配置"),
                         QString());

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(0);

    m_offlineSection = new QWidget(content);
    m_offlineSection->setObjectName(QStringLiteral("OfflineIdentitySection"));
    auto* offlineLayout = new QVBoxLayout(m_offlineSection);
    offlineLayout->setContentsMargins(0, 0, 0, 0);
    offlineLayout->setSpacing(0);

    auto* identityTitle = new QLabel(QStringLiteral("本地身份"), m_offlineSection);
    identityTitle->setObjectName(QStringLiteral("SectionTitle"));
    offlineLayout->addWidget(identityTitle);
    offlineLayout->addSpacing(4);
    m_identityNote = new QLabel(
        QStringLiteral("离线名仅用于本地单机游戏，不是微软正版登录。"
                       "改名会改变离线 UUID，也会影响已有世界中该玩家的身份与物品归属。"),
        m_offlineSection);
    m_identityNote->setObjectName(QStringLiteral("DetailNote"));
    m_identityNote->setWordWrap(true);
    offlineLayout->addWidget(m_identityNote);
    offlineLayout->addSpacing(10);

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
    layout->addWidget(m_offlineSection);

    m_accountSection = new QWidget(content);
    m_accountSection->setObjectName(QStringLiteral("AccountPanel"));
    auto* accountLayout = new QVBoxLayout(m_accountSection);
    accountLayout->setContentsMargins(0, 0, 0, 0);
    accountLayout->setSpacing(0);

    auto* accountTitle = new QLabel(QStringLiteral("微软账号"), m_accountSection);
    accountTitle->setObjectName(QStringLiteral("SectionTitle"));
    accountLayout->addWidget(accountTitle);
    accountLayout->addSpacing(4);
    m_accountNote = new QLabel(
        QStringLiteral("使用自己的微软账号登录；登录成功只更新账号状态，游戏仍需手动启动。"),
        m_accountSection);
    m_accountNote->setObjectName(QStringLiteral("DetailNote"));
    m_accountNote->setWordWrap(true);
    accountLayout->addWidget(m_accountNote);
    accountLayout->addSpacing(8);

    m_accountName = new QLabel(m_accountSection);
    m_accountName->setObjectName(QStringLiteral("SettingsAccountName"));
    m_accountName->setTextFormat(Qt::PlainText);
    m_accountName->setWordWrap(true);
    accountLayout->addWidget(m_accountName);
    m_accountMeta = new QLabel(m_accountSection);
    m_accountMeta->setObjectName(QStringLiteral("SettingsAccountState"));
    m_accountMeta->setTextFormat(Qt::PlainText);
    m_accountMeta->setWordWrap(true);
    accountLayout->addWidget(m_accountMeta);
    accountLayout->addSpacing(4);
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
    accountLayout->addSpacing(8);

    auto* accountButtons = new QHBoxLayout();
    accountButtons->setContentsMargins(0, 0, 0, 0);
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
    layout->addWidget(m_accountSection);

    auto* divider = new QFrame(content);
    divider->setObjectName(QStringLiteral("Divider"));
    divider->setFixedHeight(1);
    layout->addSpacing(16);
    layout->addWidget(divider);
    layout->addSpacing(14);

    auto* runtimeTitle = new QLabel(QStringLiteral("运行配置"), content);
    runtimeTitle->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(runtimeTitle);
    layout->addSpacing(4);
    auto* runtimeNote = new QLabel(
        QStringLiteral("可在首页选择原版版本或导入整合包；安装完成后请手动启动游戏。"),
        content);
    runtimeNote->setObjectName(QStringLiteral("DetailNote"));
    runtimeNote->setWordWrap(true);
    layout->addWidget(runtimeNote);
    layout->addSpacing(6);
    m_memoryRow = new SummaryRow(content);
    m_versionRow = new SummaryRow(content);
    m_javaRow = new SummaryRow(content);
    layout->addWidget(m_memoryRow);
    layout->addWidget(m_versionRow);
    layout->addWidget(m_javaRow);

    auto* dataLabel = new QLabel(QStringLiteral("数据目录"), content);
    dataLabel->setObjectName(QStringLiteral("DetailLabel"));
    layout->addSpacing(8);
    layout->addWidget(dataLabel);
    m_dataPath = new ElidedLabel(content);
    m_dataPath->setObjectName(QStringLiteral("PathValue"));
    m_dataPath->setFixedHeight(36);
    layout->addWidget(m_dataPath);

    layout->addStretch(1);

    setContent(content);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, [this]() {
        if (m_offlineMode) {
            clearFieldError();
            emit saveNameRequested(m_nameInput->text());
        }
    });
    connect(m_nameInput, &QLineEdit::textEdited, this, [this]() {
        m_edited = true;
        clearFieldError();
        m_savedNote->hide();
    });
    connect(m_nameInput, &QLineEdit::returnPressed, this, [this]() {
        if (m_offlineMode && m_actionBar->primaryButton()->isEnabled()) {
            m_actionBar->primaryButton()->click();
        }
    });
    connect(m_signInButton, &QPushButton::clicked, this, &SettingsPage::signInRequested);
    connect(m_cancelSignInButton, &QPushButton::clicked, this,
            &SettingsPage::cancelSignInRequested);
    connect(m_signOutButton, &QPushButton::clicked, this, &SettingsPage::signOutRequested);

    setSnapshot({});
}

void SettingsPage::setSnapshot(const application::HomeSnapshot& home)
{
    m_home = home;
    m_sessionActive = home.sessionActive;
    m_offlineMode = home.offlineMode;

    if (m_offlineMode) {
        header()->setContent(QStringLiteral("设置"),
                             QStringLiteral("本地离线身份与运行配置"),
                             QStringLiteral("本地离线"));
        const QString name = home.account.displayName;
        if (!m_edited && m_nameInput->text() != name) {
            m_nameInput->setText(name);
        }
    } else {
        header()->setContent(QStringLiteral("设置"),
                             QStringLiteral("微软账号与运行配置"),
                             QStringLiteral("微软账号"));
    }

    m_memoryRow->setData(QStringLiteral("内存上限"),
                         QStringLiteral("2048 MB（当前固定）"));
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

void SettingsPage::setSaveResult(bool accepted, const QString& message)
{
    if (accepted) {
        m_edited = false;
        m_fieldError->hide();
        m_savedNote->setText(message.isEmpty()
                                 ? QStringLiteral("离线名已应用。")
                                 : message);
        m_savedNote->show();
        m_actionBar->setErrorState(false);
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

void SettingsPage::applyEnabledState()
{
    m_offlineSection->setVisible(m_offlineMode);
    m_accountSection->setVisible(!m_offlineMode);
    if (m_offlineMode) {
        const bool editable = !m_sessionActive;
        m_nameInput->setEnabled(editable);
        m_actionBar->setPrimary(QStringLiteral("保存离线名"), editable,
                                editable ? QStringLiteral("保存后立即应用到本次启动器。")
                                         : QStringLiteral("游戏运行中不能修改离线身份。"));
        m_actionBar->setPrimaryVisible(true);
        m_actionBar->setSecondary(QString(), false, false, {});
        if (m_sessionActive) {
            m_actionBar->setTitle(QStringLiteral("游戏运行中"));
            m_actionBar->setNote(QStringLiteral("请先退出游戏，再修改离线身份。"));
            m_identityNote->setText(
                QStringLiteral("游戏运行中不能修改离线身份。关闭启动器窗口不会结束游戏。"));
        } else {
            m_actionBar->setTitle(QStringLiteral("本地设置"));
            m_actionBar->setNote(QStringLiteral("保存只修改离线身份，不会开始游戏或下载文件。"));
            m_identityNote->setText(
                QStringLiteral("离线名仅用于本地单机游戏，不是微软正版登录。"
                               "改名会改变离线 UUID，也会影响已有世界中该玩家的身份与物品归属。"));
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

    m_actionBar->setTitle(QStringLiteral("账号与启动"));
    m_actionBar->setNote(sessionActive
                             ? QStringLiteral("游戏运行中不能登录、取消登录或退出账号。")
                             : QStringLiteral("登录成功只更新账号状态，游戏仍需手动启动。"));
    m_actionBar->setPrimary(QString(), false, {});
    m_actionBar->setPrimaryVisible(false);
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

void SettingsPage::clearFieldError()
{
    m_fieldError->hide();
    m_actionBar->setErrorState(false);
}

} // namespace hustmcl::ui
