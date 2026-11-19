#include "page_header.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace hustmcl::ui {

PageHeader::PageHeader(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("PageTitle"));
    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    textLayout->addStretch(1);
    textLayout->addWidget(m_title);
    textLayout->addWidget(m_subtitle);
    textLayout->addStretch(1);
    layout->addLayout(textLayout, 1);

    auto* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addStretch(1);
    m_trailing = new QLabel(this);
    m_trailing->setObjectName(QStringLiteral("HeaderMeta"));
    m_trailing->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rightLayout->addWidget(m_trailing, 0, Qt::AlignRight | Qt::AlignVCenter);
    rightLayout->addStretch(1);
    layout->addLayout(rightLayout, 0);

    setFixedHeight(60);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void PageHeader::setContent(const QString& title, const QString& subtitle, const QString& trailing)
{
    m_title->setText(title);
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(!subtitle.isEmpty());
    m_trailing->setText(trailing);
    m_title->setAccessibleName(title);
}

void PageHeader::setTrailingText(const QString& text)
{
    m_trailing->setText(text);
}

void PageHeader::setDecorationsVisible(bool visible)
{
    Q_UNUSED(visible);
}

} // namespace hustmcl::ui
