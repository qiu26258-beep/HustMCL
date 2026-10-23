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
#include <QVBoxLayout>

namespace hustmcl::ui {

SettingsPage::SettingsPage(QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("设置"),
                         QStringLiteral("本地离线身份与固定运行配置"),
                         QStringLiteral("本地离线"));

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(0);

    auto* identityTitle = new QLabel(QStringLiteral("本地身份"), content);
    identityTitle->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(identityTitle);
    layout->addSpacing(4);
    m_identityNote = new QLabel(
        QStringLiteral("离线名仅用于本地单机游戏，不是微软正版登录。"
                       "改名会改变离线 UUID，也会影响已有世界中该玩家的身份与物品归属。"),
        content);
    m_identityNote->setObjectName(QStringLiteral("DetailNote"));
    m_identityNote->setWordWrap(true);
    layout->addWidget(m_identityNote);
    layout->addSpacing(10);

    auto* fieldLabel = new QLabel(QStringLiteral("离线玩家名"), content);
    fieldLabel->setObjectName(QStringLiteral("DetailLabel"));
    layout->addWidget(fieldLabel);
    m_nameInput = new QLineEdit(content);
    m_nameInput->setObjectName(QStringLiteral("PlayerNameInput"));
    m_nameInput->setPlaceholderText(QStringLiteral("例如 HustPlayer"));
    m_nameInput->setMaxLength(64);
    m_nameInput->setClearButtonEnabled(true);
    m_nameInput->setAccessibleName(QStringLiteral("离线玩家名"));
    m_nameInput->setToolTip(
        QStringLiteral("仅支持 1–16 位英文字母、数字或下划线。"));
    layout->addWidget(m_nameInput);
    m_fieldError = new QLabel(content);
    m_fieldError->setObjectName(QStringLiteral("FieldError"));
    m_fieldError->setWordWrap(true);
    m_fieldError->hide();
    layout->addWidget(m_fieldError);
    m_savedNote = new QLabel(content);
    m_savedNote->setObjectName(QStringLiteral("FieldSuccess"));
    m_savedNote->setWordWrap(true);
    m_savedNote->hide();
    layout->addWidget(m_savedNote);

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
        QStringLiteral("当前使用本地已安装版本；内存自定义与下载其他版本尚未实现。"),
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

    layout->addSpacing(16);
    auto* accountDivider = new QFrame(content);
    accountDivider->setObjectName(QStringLiteral("Divider"));
    accountDivider->setFixedHeight(1);
    layout->addWidget(accountDivider);
    layout->addSpacing(14);
    auto* accountTitle = new QLabel(QStringLiteral("正版登录"), content);
    accountTitle->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(accountTitle);
    layout->addSpacing(4);
    auto* accountNote = new QLabel(
        QStringLiteral("微软账号登录暂未接入。本版本提供本地离线体验。"),
        content);
    accountNote->setObjectName(QStringLiteral("DetailNote"));
    accountNote->setWordWrap(true);
    layout->addWidget(accountNote);
    layout->addStretch(1);

    setContent(content);

    m_actionBar = actionBar();
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, [this]() {
        clearFieldError();
        emit saveNameRequested(m_nameInput->text());
    });
    connect(m_nameInput, &QLineEdit::textEdited, this, [this]() {
        m_edited = true;
        clearFieldError();
        m_savedNote->hide();
    });
    connect(m_nameInput, &QLineEdit::returnPressed, this, [this]() {
        if (m_actionBar->primaryButton()->isEnabled()) {
            m_actionBar->primaryButton()->click();
        }
    });

    setSnapshot({});
}

void SettingsPage::setSnapshot(const application::HomeSnapshot& home)
{
    m_home = home;
    m_sessionActive = home.sessionActive;

    const QString name = home.account.displayName;
    if (!m_edited) {
        if (m_nameInput->text() != name) {
            m_nameInput->setText(name);
        }
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

    applyEnabledState();
}

void SettingsPage::setSaveResult(bool accepted, const QString& message)
{
    if (accepted) {
        m_edited = false;
        m_fieldError->hide();
        m_savedNote->setText(message.isEmpty()
                                 ? QStringLiteral("离线名已保存。")
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
    m_nameInput->setFocus(Qt::OtherFocusReason);
}

void SettingsPage::focusNameField()
{
    if (m_nameInput->isEnabled()) {
        m_nameInput->setFocus(Qt::OtherFocusReason);
        m_nameInput->selectAll();
        return;
    }
    if (m_actionBar->primaryButton()->isVisible()
        && m_actionBar->primaryButton()->isEnabled()) {
        m_actionBar->primaryButton()->setFocus(Qt::OtherFocusReason);
    }
}

void SettingsPage::applyEnabledState()
{
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
}

void SettingsPage::clearFieldError()
{
    m_fieldError->hide();
    m_actionBar->setErrorState(false);
}

} // namespace hustmcl::ui
