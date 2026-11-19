#include "combo_box.h"

#include "theme.h"

#include <QColor>
#include <QPainter>
#include <QPaintEvent>

namespace hustmcl::ui {

ComboBox::ComboBox(QWidget* parent)
    : QComboBox(parent)
{
}

void ComboBox::paintEvent(QPaintEvent* event)
{
    QComboBox::paintEvent(event);

    // Crisp pixel-stepped Minecraft downward chevron
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const auto& pal = theme::currentPalette();
    const QColor ink = theme::color(isEnabled() ? pal.inkSoft : pal.disabledText);
    const int cx = width() - 14;
    const int cy = height() / 2;

    painter.fillRect(QRect(cx - 4, cy - 2, 9, 2), ink);
    painter.fillRect(QRect(cx - 2, cy, 5, 2), ink);
    painter.fillRect(QRect(cx, cy + 2, 1, 1), ink);
}

} // namespace hustmcl::ui
