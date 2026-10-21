#include "sidebar.h"

#include "campus_artwork.h"
#include "copy.h"
#include "theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QStringList>
#include <QVBoxLayout>

namespace hustmcl::ui {

namespace {
constexpr int kPageHome = 0;
constexpr int kPageInstall = 1;
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

    auto* demoBadge = new QLabel(QStringLiteral("界面演示 · 样例数据"), this);
    demoBadge->setObjectName(QStringLiteral("DemoBadge"));
    demoBadge->setAlignment(Qt::AlignCenter);
    demoBadge->setToolTip(QStringLiteral("当前窗口只演示页面状态，未接入真实登录、下载或游戏启动。"));
    layout->addWidget(demoBadge);

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
    m_nav->setFixedHeight(4 * 46);
    const QStringList labels = {
        QStringLiteral("01    首页"),
        QStringLiteral("02    安装游戏"),
        QStringLiteral("03    设置"),
        QStringLiteral("04    日志与错误"),
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

    m_caption = new QLabel(QStringLiteral("小科陪你，慢慢准备。"), this);
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

    auto* demoNote = new QLabel(QStringLiteral("登录、下载与启动尚未接入"), this);
    demoNote->setObjectName(QStringLiteral("DemoNote"));
    demoNote->setWordWrap(true);
    layout->addWidget(demoNote);

    auto* foot = new QLabel(QStringLiteral("HustMCL 0.1 · 界面演示"), this);
    foot->setObjectName(QStringLiteral("RailFoot"));
    layout->addWidget(foot);

    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row == kPageHome || row == kPageInstall) {
            emit pageActivated(row);
        }
    });
    connect(m_accountLink, &QPushButton::clicked, this, [this]() {
        emit unimplementedFeatureRequested(QStringLiteral("账号与授权"));
    });
}

void Sidebar::setAccount(const application::AccountSnapshot& account)
{
    const QString name = account.displayName.isEmpty() ? QStringLiteral("未登录") : account.displayName;
    m_accountName->setText(name);
    m_avatar->setText(name.left(1));
    m_accountState->setText(copy::accountStateText(account.state));
    const bool signedIn = account.state == application::AccountState::SignedIn;
    m_accountState->setProperty("ok", signedIn);
    m_accountState->style()->unpolish(m_accountState);
    m_accountState->style()->polish(m_accountState);
    m_accountLink->setText(signedIn ? QStringLiteral("账号与授权 →") : QStringLiteral("登录账号 →"));
}

void Sidebar::setCurrentPage(int pageIndex)
{
    if (pageIndex != kPageHome && pageIndex != kPageInstall) {
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
