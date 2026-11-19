#include "combo_box.h"

#include "theme.h"

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QString>

namespace hustmcl::ui {

ComboBox::ComboBox(QWidget* parent)
    : QComboBox(parent)
{
}

void ComboBox::paintEvent(QPaintEvent* event)
{
    QComboBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal arrowWidth = 8.0;
    const qreal arrowHeight = 5.0;
    const qreal right = width() - 14.0;
    const qreal centerY = height() / 2.0;

    QPainterPath arrow;
    arrow.moveTo(right - arrowWidth, centerY - arrowHeight / 2.0);
    arrow.lineTo(right, centerY - arrowHeight / 2.0);
    arrow.lineTo(right - arrowWidth / 2.0, centerY + arrowHeight / 2.0);
    arrow.closeSubpath();

    painter.fillPath(
        arrow,
        QColor(QString::fromLatin1(isEnabled() ? theme::kMuted : theme::kDisabledText)));
}

} // namespace hustmcl::ui
