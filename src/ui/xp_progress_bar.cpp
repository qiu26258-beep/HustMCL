#include "xp_progress_bar.h"

#include "theme.h"

#include <QPainter>
#include <QPaintEvent>

namespace hustmcl::ui {

XpProgressBar::XpProgressBar(QWidget* parent)
    : QProgressBar(parent)
{
    setTextVisible(false);
    setMinimumHeight(14);
    setMaximumHeight(14);
}

void XpProgressBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const auto& pal = theme::currentPalette();
    const QRect outer = rect();
    if (outer.width() <= 4 || outer.height() <= 4) {
        return;
    }

    // Obsidian XP bar frame
    painter.fillRect(outer, theme::color(pal.xpBorder));
    const QRect inner = outer.adjusted(2, 2, -2, -2);
    painter.fillRect(inner, theme::color(pal.xpTrack));

    // Subtle top shadow inside track
    painter.fillRect(QRect(inner.left(), inner.top(), inner.width(), 1), QColor(0, 0, 0, 120));

    const int minVal = minimum();
    const int maxVal = maximum();
    if (maxVal > minVal) {
        const qreal ratio = qBound(0.0,
                                   static_cast<qreal>(value() - minVal)
                                       / static_cast<qreal>(maxVal - minVal),
                                   1.0);
        const int fillWidth = qRound(inner.width() * ratio);
        if (fillWidth > 0) {
            const QRect fillRect(inner.left(), inner.top(), fillWidth, inner.height());
            painter.fillRect(fillRect, QColor(QStringLiteral("#3CC94A")));
            // Bright lime top highlight strip (authentic MC XP bar sheen)
            const int hiH = qMax(2, inner.height() / 3);
            painter.fillRect(QRect(fillRect.left(), fillRect.top(), fillRect.width(), hiH),
                             QColor(QStringLiteral("#8CF76F")));
            // Deep emerald bottom edge
            painter.fillRect(QRect(fillRect.left(), fillRect.bottom() - 1, fillRect.width(), 2),
                             QColor(QStringLiteral("#1D7A2E")));
        }
    } else {
        // Indeterminate: crisp static pixel blocks (no fake percentage, zero animation CPU)
        const int block = 12;
        for (int x = inner.left(); x < inner.right(); x += block * 2) {
            const int w = qMin(block, inner.right() - x + 1);
            painter.fillRect(QRect(x, inner.top(), w, inner.height()),
                             QColor(QStringLiteral("#2D8C46")));
        }
    }

    // 10-segment Minecraft XP bar pixel notches
    constexpr int kSegments = 10;
    const QColor notchColor(0, 0, 0, 145);
    for (int i = 1; i < kSegments; ++i) {
        const int x = inner.left() + (inner.width() * i) / kSegments;
        painter.fillRect(QRect(x, inner.top(), 1, inner.height()), notchColor);
    }
}

} // namespace hustmcl::ui
