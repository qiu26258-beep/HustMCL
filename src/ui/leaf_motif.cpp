#include "leaf_motif.h"

#include <QPainter>
#include <QPainterPath>

namespace hustmcl::ui {

LeafMotif::LeafMotif(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setFixedSize(160, 64);
}

void LeafMotif::setDecorationVisible(bool visible)
{
    if (m_decorationVisible == visible) {
        return;
    }
    m_decorationVisible = visible;
    update();
}

bool LeafMotif::decorationVisible() const
{
    return m_decorationVisible;
}

QSize LeafMotif::sizeHint() const
{
    return QSize(160, 64);
}

void LeafMotif::paintEvent(QPaintEvent*)
{
    if (!m_decorationVisible) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto drawLeaf = [&painter](const QPointF& origin, qreal rotation, qreal scale, int alpha) {
        painter.save();
        painter.translate(origin);
        painter.rotate(rotation);
        painter.scale(scale, scale);
        QPainterPath leaf;
        leaf.moveTo(0.0, 0.0);
        leaf.cubicTo(7.0, -9.0, 20.0, -11.0, 31.0, -2.0);
        leaf.cubicTo(20.0, 8.0, 7.0, 7.0, 0.0, 0.0);
        leaf.moveTo(2.0, 0.0);
        leaf.lineTo(25.0, -1.0);
        QColor color(QStringLiteral("#CCD8C4"));
        color.setAlpha(alpha);
        painter.setPen(QPen(QColor(QStringLiteral("#9BAE9A")), 1.1));
        painter.setBrush(color);
        painter.drawPath(leaf);
        painter.restore();
    };

    drawLeaf(QPointF(106.0, 28.0), -24.0, 1.0, 110);
    drawLeaf(QPointF(88.0, 46.0), 18.0, 0.78, 82);
    drawLeaf(QPointF(132.0, 44.0), 52.0, 0.66, 70);
    drawLeaf(QPointF(74.0, 22.0), -58.0, 0.58, 58);
}

} // namespace hustmcl::ui
