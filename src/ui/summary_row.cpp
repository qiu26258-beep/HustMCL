#include "summary_row.h"

#include <QHBoxLayout>
#include <QLabel>

namespace hustmcl::ui {

SummaryRow::SummaryRow(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    m_label = new QLabel(this);
    m_label->setObjectName(QStringLiteral("SummaryLabel"));
    m_value = new QLabel(this);
    m_value->setObjectName(QStringLiteral("SummaryValue"));
    m_value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_value->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(m_label, 1);
    layout->addWidget(m_value, 0);
    setMinimumHeight(30);
}

void SummaryRow::setData(const QString& label, const QString& value)
{
    m_label->setText(label);
    m_value->setText(value);
    m_value->setToolTip(value);
    setAccessibleName(QStringLiteral("%1，%2").arg(label, value));
}

} // namespace hustmcl::ui
