#include "primary_action_bar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace hustmcl::ui {

PrimaryActionBar::PrimaryActionBar(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ActionBar"));
    setProperty("error", false);
    setMinimumHeight(80);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 12, 24, 12);
    layout->setSpacing(16);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);
    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("ActionTitle"));
    m_title->setProperty("error", false);
    m_note = new QLabel(this);
    m_note->setObjectName(QStringLiteral("ActionNote"));
    m_note->setWordWrap(true);
    textLayout->addWidget(m_title);
    textLayout->addWidget(m_note);
    textLayout->addStretch(1);
    layout->addLayout(textLayout, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(18);
    m_secondary = new QPushButton(this);
    m_secondary->setObjectName(QStringLiteral("SecondaryAction"));
    m_secondary->setCursor(Qt::PointingHandCursor);
    m_secondary->setAutoDefault(false);
    m_secondary->setDefault(false);
    m_secondary->hide();
    m_primary = new QPushButton(this);
    m_primary->setObjectName(QStringLiteral("PrimaryAction"));
    m_primary->setCursor(Qt::PointingHandCursor);
    m_primary->setAutoDefault(false);
    m_primary->setDefault(false);
    m_primary->hide();
    buttonLayout->addWidget(m_secondary, 0, Qt::AlignVCenter);
    buttonLayout->addWidget(m_primary, 0, Qt::AlignVCenter);
    layout->addLayout(buttonLayout, 0);

    connect(m_primary, &QPushButton::clicked, this, &PrimaryActionBar::primaryClicked);
    connect(m_secondary, &QPushButton::clicked, this, &PrimaryActionBar::secondaryClicked);
}

void PrimaryActionBar::setTitle(const QString& title)
{
    m_title->setText(title);
}

void PrimaryActionBar::setNote(const QString& note)
{
    m_note->setText(note);
    m_note->setVisible(!note.isEmpty());
}

void PrimaryActionBar::setPrimary(const QString& text, bool enabled, const QString& tooltip)
{
    m_primary->setText(text);
    m_primary->setEnabled(enabled);
    m_primary->setToolTip(tooltip);
    m_primary->setAccessibleName(text);
    m_primary->setAccessibleDescription(tooltip);
    m_primary->setVisible(!text.isEmpty());
}

void PrimaryActionBar::setSecondary(const QString& text, bool visible, bool enabled,
                                    const QString& tooltip, bool danger)
{
    const QString objectName = danger ? QStringLiteral("CancelAction") : QStringLiteral("SecondaryAction");
    if (m_secondary->objectName() != objectName) {
        m_secondary->setObjectName(objectName);
        repolish(m_secondary);
    }
    m_secondary->setText(text);
    m_secondary->setVisible(visible);
    m_secondary->setEnabled(enabled);
    m_secondary->setToolTip(tooltip);
    m_secondary->setAccessibleName(text);
    m_secondary->setAccessibleDescription(tooltip);
}

void PrimaryActionBar::setPrimaryVisible(bool visible)
{
    m_primary->setVisible(visible);
}

void PrimaryActionBar::setSecondaryVisible(bool visible)
{
    m_secondary->setVisible(visible);
}

void PrimaryActionBar::setErrorState(bool error)
{
    if (property("error").toBool() == error) {
        return;
    }
    setProperty("error", error);
    m_title->setProperty("error", error);
    repolish(this);
    repolish(m_title);
}

QPushButton* PrimaryActionBar::primaryButton() const
{
    return m_primary;
}

QPushButton* PrimaryActionBar::secondaryButton() const
{
    return m_secondary;
}

void PrimaryActionBar::repolish(QWidget* widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace hustmcl::ui
