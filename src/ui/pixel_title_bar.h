#pragma once

#include "theme.h"

#include <QFrame>
#include <QPoint>

class QLabel;
class QMouseEvent;
class QPushButton;

namespace hustmcl::ui {

// Hardcore Minecraft pixel-styled window title bar modeled on Windows native
// conventions: left emblem + title, right HUST coordinates + quick theme
// switcher + native-order Minimize / Maximize-Restore / Close pixel buttons.
class PixelTitleBar : public QFrame {
    Q_OBJECT

public:
    explicit PixelTitleBar(QWidget* parent = nullptr);

    void setTitleText(const QString& title, const QString& subtitle = QString());
    void setThemeState(theme::ThemeMode mode, theme::ResolvedTheme resolved);
    void setMaximized(bool maximized);
    void setCompact(bool compact);
    void refreshIcons();

    QPushButton* themeButton() const;
    QPushButton* minimizeButton() const;
    QPushButton* maximizeButton() const;
    QPushButton* closeButton() const;

signals:
    void themeCycleRequested();
    void minimizeRequested();
    void maximizeToggleRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QLabel* m_icon = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_coord = nullptr;
    QPushButton* m_themeBtn = nullptr;
    QPushButton* m_minBtn = nullptr;
    QPushButton* m_maxBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
    theme::ThemeMode m_themeMode = theme::ThemeMode::AutoByTime;
    theme::ResolvedTheme m_resolvedTheme = theme::ResolvedTheme::Dark;
    bool m_maximized = false;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

} // namespace hustmcl::ui
