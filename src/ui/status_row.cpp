#include "status_row.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

namespace hustmcl::ui {

StatusRow::StatusRow(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    m_check = new QLabel(this);
    m_check->setObjectName(QStringLiteral("StatusCheck"));
    m_check->setProperty("ok", true);
    m_check->setAlignment(Qt::AlignCenter);
    m_name = new QLabel(this);
    m_name->setObjectName(QStringLiteral("StatusName"));
    m_name->setMinimumWidth(84);
    m_value = new QLabel(this);
    m_value->setObjectName(QStringLiteral("StatusValue"));
    m_value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_state = new QLabel(this);
    m_state->setObjectName(QStringLiteral("StatusState"));
    m_state->setProperty("ok", true);
    m_state->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(m_check, 0, Qt::AlignVCenter);
    layout->addWidget(m_name, 0, Qt::AlignVCenter);
    layout->addWidget(m_value, 1, Qt::AlignVCenter);
    layout->addWidget(m_state, 0, Qt::AlignVCenter);
    setMinimumHeight(46);
}

void StatusRow::setData(const QString& name, const QString& value, const QString& state, bool ok)
{
    m_name->setText(name);
    m_value->setText(value);
    m_value->setToolTip(value);
    m_state->setText(state);
    m_check->setText(ok ? QStringLiteral("✓") : QStringLiteral("!"));
    setState(ok);
    setAccessibleName(QStringLiteral("%1，%2，%3").arg(name, value, state));
}

void StatusRow::setState(bool ok)
{
    m_check->setProperty("ok", ok);
    m_state->setProperty("ok", ok);
    repolish();
}

void StatusRow::repolish()
{
    for (QLabel* label : {m_check, m_state}) {
        label->style()->unpolish(label);
        label->style()->polish(label);
        label->update();
    }
}

} // namespace hustmcl::ui
