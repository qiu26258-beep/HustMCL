#include "page_scaffold.h"

#include "page_header.h"
#include "primary_action_bar.h"

#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>

namespace hustmcl::ui {

PageScaffold::PageScaffold(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(16);

    m_header = new PageHeader(this);
    layout->addWidget(m_header, 0);

    m_desk = new QFrame(this);
    m_desk->setObjectName(QStringLiteral("Desk"));
    auto* deskLayout = new QVBoxLayout(m_desk);
    deskLayout->setContentsMargins(0, 0, 0, 0);
    deskLayout->setSpacing(0);

    m_scroll = new QScrollArea(m_desk);
    m_scroll->setObjectName(QStringLiteral("DeskScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    deskLayout->addWidget(m_scroll, 1);

    m_actionBar = new PrimaryActionBar(m_desk);
    deskLayout->addWidget(m_actionBar, 0);
    layout->addWidget(m_desk, 1);
}

PageHeader* PageScaffold::header() const
{
    return m_header;
}

PrimaryActionBar* PageScaffold::actionBar() const
{
    return m_actionBar;
}

QScrollArea* PageScaffold::scrollArea() const
{
    return m_scroll;
}

void PageScaffold::setContent(QWidget* content)
{
    content->setObjectName(QStringLiteral("DeskContent"));
    content->setAutoFillBackground(false);
    m_scroll->setWidget(content);
}

void PageScaffold::setDecorationsVisible(bool visible)
{
    m_header->setDecorationsVisible(visible);
}

} // namespace hustmcl::ui
