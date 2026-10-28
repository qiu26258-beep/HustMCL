#include "sidebar.h"

#include "campus_artwork.h"
#include "copy.h"
#include "theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QStringList>
#include <QVBoxLayout>

namespace hustmcl::ui {

namespace {
constexpr int kPageHome = 0;
constexpr int kPageInstall = 1;
constexpr int kPageSettings = 2;
constexpr int kPageLogs = 3;
constexpr int kPageMarket = 4;
}

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Sidebar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(200);
    setMaximumWidth(216);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(6);

    auto* brandRow = new QHBoxLayout();
    brandRow->setContentsMargins(0, 0, 0, 0);
    brandRow->setSpacing(10);
    auto* mark = new QLabel(QStringLiteral("H"), this);
    mark->setObjectName(QStringLiteral("BrandMark"));
    mark->setAlignment(Qt::AlignCenter);
    {
        // The product icon doubles as the brand mark; "H" stays as the fallback.
        QPixmap icon(QStringLiteral(":/brand/hustmcl-128.png"));
        if (!icon.isNull()) {
            const qreal ratio = mark->devicePixelRatioF();
            icon = icon.scaled(QSize(38, 38) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            icon.setDevicePixelRatio(ratio);
            mark->setPixmap(icon);
        }
    }
    auto* brandText = new QVBoxLayout();
    brandText->setContentsMargins(0, 0, 0, 0);
    brandText->setSpacing(1);
    auto* name = new QLabel(QStringLiteral("HustMCL"), this);
    name->setObjectName(QStringLiteral("BrandName"));
    auto* sub = new QLabel(QStringLiteral("校园启动器"), this);
    sub->setObjectName(QStringLiteral("BrandSub"));
    brandText->addWidget(name);
    brandText->addWidget(sub);
    brandRow->addWidget(mark, 0, Qt::AlignVCenter);
    brandRow->addLayout(brandText, 1);
    layout->addLayout(brandRow);

    m_badge = new QLabel(QStringLiteral("界面演示 · 样例数据"), this);
    m_badge->setObjectName(QStringLiteral("DemoBadge"));
    m_badge->setAlignment(Qt::AlignCenter);
    m_badge->setToolTip(QStringLiteral("当前窗口只演示页面状态，不执行真实账号、下载或游戏启动操作。"));
    layout->addWidget(m_badge);

    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("Divider"));
    divider->setFixedHeight(1);
    layout->addWidget(divider);

    m_nav = new QListWidget(this);
    m_nav->setObjectName(QStringLiteral("Navigation"));
    m_nav->setAccessibleName(QStringLiteral("主导航"));
    m_nav->setFocusPolicy(Qt::StrongFocus);
    m_nav->setSelectionMode(QAbstractItemView::SingleSelection);
    m_nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->setFixedHeight(5 * 46);
    const QStringList labels = {
        QStringLiteral("01    首页"),
        QStringLiteral("02    安装游戏"),
        QStringLiteral("03    设置"),
        QStringLiteral("04    日志与错误"),
        QStringLiteral("05    模组市场"),
    };
    for (int i = 0; i < labels.size(); ++i) {
        auto* item = new QListWidgetItem(labels.at(i), m_nav);
        item->setSizeHint(QSize(0, 46));
        if (i > kPageInstall) {
            item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            item->setToolTip(QStringLiteral("该页面将在后续批次提供；当前仅演示首页与安装页。"));
        }
    }
    m_nav->setCurrentRow(kPageHome);
    layout->addWidget(m_nav);

    m_navNote = new QLabel(QStringLiteral("设置与日志将在后续批次提供"), this);
    m_navNote->setObjectName(QStringLiteral("NavNote"));
    m_navNote->setWordWrap(true);
    layout->addWidget(m_navNote);
    m_artwork = new CampusArtwork(this);
    m_artwork->setObjectName(QStringLiteral("CampusArtwork"));
    m_artwork->setOverlayColor(QColor(QString::fromLatin1(theme::kRail)));
    m_artwork->setFixedHeight(180);
    m_artwork->setMinimumWidth(160);
    m_artwork->hide();
    layout->addWidget(m_artwork);

#ifdef HUSTMCL_PUBLIC_RELEASE
    m_caption = new QLabel(QStringLiteral("校园的树影下，慢慢准备。"), this);
#else
    m_caption = new QLabel(QStringLiteral("小科陪你，慢慢准备。"), this);
#endif
    m_caption->setObjectName(QStringLiteral("SidebarCaption"));
    m_caption->setWordWrap(true);
    m_caption->hide();
    layout->addWidget(m_caption);

    m_subCaption = new QLabel(QStringLiteral("树荫下的片刻，也是冒险的起点。"), this);
    m_subCaption->setObjectName(QStringLiteral("SidebarSub"));
    m_subCaption->setWordWrap(true);
    m_subCaption->hide();
    layout->addWidget(m_subCaption);

    layout->addStretch(1);

    auto* accountDivider = new QFrame(this);
    accountDivider->setObjectName(QStringLiteral("Divider"));
    accountDivider->setFixedHeight(1);
    layout->addWidget(accountDivider);

    auto* accountRow = new QHBoxLayout();
    accountRow->setContentsMargins(0, 0, 0, 0);
    accountRow->setSpacing(10);
    m_avatar = new QLabel(QStringLiteral("林"), this);
    m_avatar->setObjectName(QStringLiteral("Avatar"));
    m_avatar->setAlignment(Qt::AlignCenter);
    auto* accountText = new QVBoxLayout();
    accountText->setContentsMargins(0, 0, 0, 0);
    accountText->setSpacing(2);
    m_accountName = new QLabel(QStringLiteral("林知远"), this);
    m_accountName->setObjectName(QStringLiteral("AccountName"));
    m_accountState = new QLabel(QStringLiteral("已连接"), this);
    m_accountState->setObjectName(QStringLiteral("AccountState"));
    accountText->addWidget(m_accountName);
    accountText->addWidget(m_accountState);
    accountRow->addWidget(m_avatar, 0, Qt::AlignVCenter);
    accountRow->addLayout(accountText, 1);
    layout->addLayout(accountRow);

    m_accountLink = new QPushButton(QStringLiteral("账号与授权 →"), this);
    m_accountLink->setObjectName(QStringLiteral("SecondaryAction"));
    m_accountLink->setCursor(Qt::PointingHandCursor);
    m_accountLink->setAutoDefault(false);
    m_accountLink->setAccessibleName(QStringLiteral("账号与授权"));
    layout->addWidget(m_accountLink);

    m_accountNote = new QLabel(QStringLiteral("演示窗口不会执行账号、下载或启动操作"), this);
    m_accountNote->setObjectName(QStringLiteral("DemoNote"));
    m_accountNote->setWordWrap(true);
    layout->addWidget(m_accountNote);

    m_footer = new QLabel(QStringLiteral("HustMCL 0.1 · 界面演示"), this);
    m_footer->setObjectName(QStringLiteral("RailFoot"));
    layout->addWidget(m_footer);

    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || row >= m_nav->count()) {
            return;
        }
        if (m_nav->item(row)->flags() & Qt::ItemIsEnabled) {
            emit pageActivated(row);
        }
    });
    connect(m_accountLink, &QPushButton::clicked, this, [this]() {
        if (m_realMode) {
            emit accountSettingsRequested();
        } else {
            emit unimplementedFeatureRequested(QStringLiteral("账号与授权"));
        }
    });
    updateMarketItem();
}

void Sidebar::setRealMode(bool realMode)
{
    m_realMode = realMode;
    if (realMode) {
        m_nav->item(kPageInstall)->setText(QStringLiteral("02    实例与检查"));
        m_nav->item(kPageSettings)->setText(QStringLiteral("03    设置"));
        m_nav->item(kPageLogs)->setText(QStringLiteral("04    日志与错误"));
        m_nav->item(kPageMarket)->setText(QStringLiteral("05    模组市场"));
        for (int row = kPageSettings; row <= kPageLogs; ++row) {
            auto* item = m_nav->item(row);
            item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            item->setToolTip(QString());
        }
        updateMarketItem();
        m_navNote->setText(QStringLiteral("首次启动会检查本地文件并检测 Java"));
        m_navNote->setToolTip(
            QStringLiteral("启动前会先确认文件与 Java，准备好后再打开游戏。"));
        applyModeCopy();
        updateAccountPresentation();
    } else {
        m_badge->setText(QStringLiteral("界面演示 · 样例数据"));
        m_badge->setToolTip(
            QStringLiteral("当前窗口只演示页面状态，不执行真实账号、下载或游戏启动操作。"));
        m_nav->item(kPageInstall)->setText(QStringLiteral("02    安装游戏"));
        for (int row = kPageSettings; row <= kPageLogs; ++row) {
            auto* item = m_nav->item(row);
            item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            item->setToolTip(QStringLiteral("该页面将在后续批次提供；当前仅演示首页与安装页。"));
        }
        updateMarketItem();
        m_navNote->setText(QStringLiteral("设置与日志将在后续批次提供"));
        m_navNote->setToolTip(QString());
        m_accountNote->setText(QStringLiteral("演示窗口不会执行账号、下载或启动操作"));
        m_footer->setText(QStringLiteral("HustMCL 0.1 · 界面演示"));
        updateAccountLink();
    }
}

bool Sidebar::isRealMode() const
{
    return m_realMode;
}

void Sidebar::setMarketAvailable(bool available)
{
    m_marketAvailable = available;
    updateMarketItem();
}

void Sidebar::updateMarketItem()
{
    if (!m_nav || m_nav->count() <= kPageMarket) {
        return;
    }
    auto* item = m_nav->item(kPageMarket);
    if (m_realMode && m_marketAvailable) {
        item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setToolTip(QStringLiteral("浏览 Modrinth 模组并下载文件。"));
    } else {
        item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
        item->setToolTip(m_realMode
                             ? QStringLiteral("市场服务暂不可用。")
                             : QStringLiteral("模组市场在当前演示中不可用。"));
    }
}

void Sidebar::setAccount(const application::AccountSnapshot& account, bool offlineMode)
{
    m_account = account;
    m_offlineMode = offlineMode;
    updateAccountPresentation();
}

void Sidebar::updateAccountPresentation()
{
    const bool offline = m_realMode && m_offlineMode;
    const QString name = m_account.displayName.isEmpty()
        ? (m_realMode ? (offline ? QStringLiteral("本地玩家") : QStringLiteral("尚未登录"))
                      : QStringLiteral("未登录"))
        : m_account.displayName;
    m_accountName->setText(name);
    m_avatar->setText(name.isEmpty() ? QStringLiteral("H") : name.left(1));
    m_accountState->setText(copy::accountStateText(m_account.state));
    const bool ready = offline
        ? m_account.state == application::AccountState::Offline
        : m_account.state == application::AccountState::SignedIn;
    m_accountState->setProperty("ok", ready);
    m_accountState->style()->unpolish(m_accountState);
    m_accountState->style()->polish(m_accountState);
    applyModeCopy();
    updateAccountLink();
}

void Sidebar::applyModeCopy()
{
    if (!m_realMode) {
        return;
    }
    if (m_offlineMode) {
        m_badge->setText(QStringLiteral("本地离线体验"));
        m_badge->setToolTip(
            QStringLiteral("本地离线单机启动；不使用微软账号，也不下载其他版本。"));
        m_accountNote->setText(QStringLiteral("离线身份仅用于本地游戏，不是正版登录。"));
        m_footer->setText(QStringLiteral("HustMCL 0.1 · 本地离线"));
        return;
    }
    m_badge->setText(QStringLiteral("微软账号"));
    m_badge->setToolTip(
        QStringLiteral("使用自己的微软账号登录；游戏仍由你手动启动。"));
    m_accountNote->setText(m_account.state == application::AccountState::SignedIn
                               ? QStringLiteral("已连接微软账号，游戏可正版启动。")
                               : QStringLiteral("登录微软账号后可使用正版启动。"));
    m_footer->setText(QStringLiteral("HustMCL 0.1 · 正版账号"));
}

void Sidebar::updateAccountLink()
{
    if (!m_accountLink) {
        return;
    }
    if (m_realMode) {
        if (m_offlineMode) {
            m_accountLink->setText(QStringLiteral("修改离线身份 →"));
            m_accountLink->setAccessibleName(QStringLiteral("修改离线身份"));
            return;
        }
        const bool signedIn = m_account.state == application::AccountState::SignedIn;
        m_accountLink->setText(signedIn ? QStringLiteral("账号与授权 →")
                                       : QStringLiteral("登录账号 →"));
        m_accountLink->setAccessibleName(signedIn ? QStringLiteral("账号与授权")
                                                  : QStringLiteral("登录账号"));
        return;
    }
    const bool signedIn = m_account.state == application::AccountState::SignedIn;
    m_accountLink->setText(signedIn ? QStringLiteral("账号与授权 →") : QStringLiteral("登录账号 →"));
}

void Sidebar::setCurrentPage(int pageIndex)
{
    const int lastEnabled = m_realMode
        ? (m_marketAvailable ? kPageMarket : kPageLogs)
        : kPageInstall;
    if (pageIndex < 0 || pageIndex > lastEnabled) {
        return;
    }
    const QSignalBlocker blocker(m_nav);
    m_nav->setCurrentRow(pageIndex);
}

void Sidebar::setArtworkVisible(bool visible)
{
    m_artworkRequested = visible;
    updateArtworkVisibility();
}

void Sidebar::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    const int itemHeight = compact ? 38 : 46;
    for (int row = 0; row < m_nav->count(); ++row) {
        m_nav->item(row)->setSizeHint(QSize(0, itemHeight));
    }
    m_nav->setFixedHeight(m_nav->count() * itemHeight);
    m_accountNote->setVisible(!compact);
    m_footer->setVisible(!compact);
    updateArtworkVisibility();
}

void Sidebar::updateArtworkVisibility()
{
    const bool visible = m_artworkRequested && !m_compact;
    m_artwork->setVisible(visible);
    m_caption->setVisible(visible);
    m_subCaption->setVisible(visible);
}

} // namespace hustmcl::ui
