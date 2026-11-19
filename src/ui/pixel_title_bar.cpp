#include "pixel_title_bar.h"

#include "voxel_icons.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWindow>

namespace hustmcl::ui {

PixelTitleBar::PixelTitleBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("PixelTitleBar"));
    setFixedHeight(38);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 6, 0);
    layout->setSpacing(8);

    m_icon = new QLabel(this);
    m_icon->setObjectName(QStringLiteral("TitleBarIcon"));
    m_icon->setFixedSize(20, 20);
    m_icon->setAlignment(Qt::AlignCenter);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("TitleBarText"));
    m_title->setAccessibleName(QStringLiteral("HustMCL"));
    m_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_subtitle = new QLabel(QStringLiteral("HUSTer的启动器"), this);
    m_subtitle->setObjectName(QStringLiteral("TitleBarSub"));

    m_coord = new QLabel(QStringLiteral("X: 114.41°E  Z: 30.51°N · 喻家山"), this);
    m_coord->setObjectName(QStringLiteral("TitleBarCoord"));

    m_themeBtn = new QPushButton(this);
    m_themeBtn->setObjectName(QStringLiteral("TitleThemeButton"));
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setAutoDefault(false);
    m_themeBtn->setDefault(false);
    m_themeBtn->setAccessibleName(QStringLiteral("切换主题模式"));
    m_themeBtn->setToolTip(QStringLiteral("点击切换：随时间自动 → 喻园深林（深色）→ 梧桐晴昼（浅色）"));

    m_minBtn = new QPushButton(this);
    m_minBtn->setObjectName(QStringLiteral("TitleWinButton"));
    m_minBtn->setCursor(Qt::PointingHandCursor);
    m_minBtn->setAutoDefault(false);
    m_minBtn->setDefault(false);
    m_minBtn->setAccessibleName(QStringLiteral("最小化"));
    m_minBtn->setToolTip(QStringLiteral("最小化"));

    m_maxBtn = new QPushButton(this);
    m_maxBtn->setObjectName(QStringLiteral("TitleWinButton"));
    m_maxBtn->setCursor(Qt::PointingHandCursor);
    m_maxBtn->setAutoDefault(false);
    m_maxBtn->setDefault(false);
    m_maxBtn->setAccessibleName(QStringLiteral("最大化"));
    m_maxBtn->setToolTip(QStringLiteral("最大化"));

    m_closeBtn = new QPushButton(this);
    m_closeBtn->setObjectName(QStringLiteral("TitleCloseButton"));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setAutoDefault(false);
    m_closeBtn->setDefault(false);
    m_closeBtn->setAccessibleName(QStringLiteral("关闭"));
    m_closeBtn->setToolTip(QStringLiteral("关闭"));

    layout->addWidget(m_icon, 0, Qt::AlignVCenter);
    layout->addWidget(m_title, 0, Qt::AlignVCenter);
    layout->addWidget(m_subtitle, 0, Qt::AlignVCenter);
    layout->addStretch(1);
    layout->addWidget(m_coord, 0, Qt::AlignVCenter);
    layout->addWidget(m_themeBtn, 0, Qt::AlignVCenter);
    layout->addSpacing(4);
    layout->addWidget(m_minBtn, 0, Qt::AlignVCenter);
    layout->addWidget(m_maxBtn, 0, Qt::AlignVCenter);
    layout->addWidget(m_closeBtn, 0, Qt::AlignVCenter);

    connect(m_themeBtn, &QPushButton::clicked, this, &PixelTitleBar::themeCycleRequested);
    connect(m_minBtn, &QPushButton::clicked, this, &PixelTitleBar::minimizeRequested);
    connect(m_maxBtn, &QPushButton::clicked, this, &PixelTitleBar::maximizeToggleRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, &PixelTitleBar::closeRequested);

    setThemeState(m_themeMode, m_resolvedTheme);
    refreshIcons();
}

void PixelTitleBar::setTitleText(const QString& title, const QString& subtitle)
{
    m_title->setAccessibleName(title.isEmpty() ? QStringLiteral("HustMCL") : title);
    m_title->setToolTip(title);
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(!subtitle.isEmpty());
}

void PixelTitleBar::setThemeState(theme::ThemeMode mode, theme::ResolvedTheme resolved)
{
    m_themeMode = mode;
    m_resolvedTheme = resolved;
    QString text;
    if (mode == theme::ThemeMode::AutoByTime) {
        text = QStringLiteral("自动·%1").arg(theme::resolvedThemeLabel(resolved));
    } else {
        text = theme::resolvedThemeLabel(resolved);
    }
    m_themeBtn->setText(text);
    refreshIcons();
}

void PixelTitleBar::setMaximized(bool maximized)
{
    if (m_maximized == maximized) {
        return;
    }
    m_maximized = maximized;
    const QString tip = maximized ? QStringLiteral("还原") : QStringLiteral("最大化");
    m_maxBtn->setToolTip(tip);
    m_maxBtn->setAccessibleName(tip);
    refreshIcons();
}

void PixelTitleBar::setCompact(bool compact)
{
    m_coord->setVisible(!compact);
    m_subtitle->setVisible(!compact && !m_subtitle->text().isEmpty());
}

void PixelTitleBar::refreshIcons()
{
    const qreal dpr = qMax<qreal>(1.0, devicePixelRatioF());
    m_icon->setPixmap(voxel::renderIcon(voxel::IconKind::SouthGateEmblem, 20, dpr));
    if (m_title) {
        const QPixmap wordmark = voxel::renderWordmark(2, dpr);
        m_title->setPixmap(wordmark);
        m_title->setFixedSize(wordmark.deviceIndependentSize().toSize());
    }

    voxel::IconKind themeIcon = voxel::IconKind::ThemeAuto;
    if (m_themeMode == theme::ThemeMode::Dark) {
        themeIcon = voxel::IconKind::ThemeDark;
    } else if (m_themeMode == theme::ThemeMode::Light) {
        themeIcon = voxel::IconKind::ThemeLight;
    }
    m_themeBtn->setIcon(voxel::icon(themeIcon, 14, dpr));
    m_themeBtn->setIconSize(QSize(14, 14));

    m_minBtn->setIcon(voxel::icon(voxel::IconKind::WinMinimize, 12, dpr));
    m_minBtn->setIconSize(QSize(12, 12));
    m_maxBtn->setIcon(voxel::icon(
        m_maximized ? voxel::IconKind::WinRestore : voxel::IconKind::WinMaximize, 12, dpr));
    m_maxBtn->setIconSize(QSize(12, 12));
    m_closeBtn->setIcon(voxel::icon(voxel::IconKind::WinClose, 12, dpr));
    m_closeBtn->setIconSize(QSize(12, 12));
}

QPushButton* PixelTitleBar::themeButton() const
{
    return m_themeBtn;
}

QPushButton* PixelTitleBar::minimizeButton() const
{
    return m_minBtn;
}

QPushButton* PixelTitleBar::maximizeButton() const
{
    return m_maxBtn;
}

QPushButton* PixelTitleBar::closeButton() const
{
    return m_closeBtn;
}

void PixelTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (QWidget* win = window()) {
            if (QWindow* handle = win->windowHandle()) {
                if (handle->startSystemMove()) {
                    event->accept();
                    return;
                }
            }
            m_dragging = true;
            m_dragOffset = event->globalPosition().toPoint() - win->frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QFrame::mousePressEvent(event);
}

void PixelTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        if (QWidget* win = window(); win && !win->isMaximized()) {
            win->move(event->globalPosition().toPoint() - m_dragOffset);
            event->accept();
            return;
        }
    }
    QFrame::mouseMoveEvent(event);
}

void PixelTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QFrame::mouseReleaseEvent(event);
}

void PixelTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeToggleRequested();
        event->accept();
        return;
    }
    QFrame::mouseDoubleClickEvent(event);
}

} // namespace hustmcl::ui
