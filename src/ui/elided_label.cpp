#include "elided_label.h"

#include <QEvent>
#include <QFontMetrics>
#include <QResizeEvent>

namespace hustmcl::ui {

ElidedLabel::ElidedLabel(QWidget* parent)
    : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setMinimumWidth(0);
}

void ElidedLabel::setFullText(const QString& text)
{
    m_fullText = text;
    setToolTip(text);
    setAccessibleName(text);
    updateElidedText();
}

QString ElidedLabel::fullText() const
{
    return m_fullText;
}

void ElidedLabel::resizeEvent(QResizeEvent* event)
{
    QLabel::resizeEvent(event);
    updateElidedText();
}

void ElidedLabel::changeEvent(QEvent* event)
{
    QLabel::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        updateElidedText();
    }
}

void ElidedLabel::updateElidedText()
{
    if (m_fullText.isEmpty()) {
        setText({});
        return;
    }
    const int available = qMax(0, width() - 20);
    if (available <= 0) {
        setText(m_fullText);
        return;
    }
    const QString elided = fontMetrics().elidedText(m_fullText, Qt::ElideMiddle, available);
    setText(elided);
}

} // namespace hustmcl::ui
