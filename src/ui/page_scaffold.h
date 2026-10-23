#pragma once

#include <QWidget>

class QFrame;
class QScrollArea;

namespace hustmcl::ui {

class PageHeader;
class PrimaryActionBar;

class PageScaffold : public QWidget {
public:
    explicit PageScaffold(QWidget* parent = nullptr);

    PageHeader* header() const;
    PrimaryActionBar* actionBar() const;
    QScrollArea* scrollArea() const;

    void setContent(QWidget* content);
    void setDecorationsVisible(bool visible);

private:
    PageHeader* m_header = nullptr;
    QFrame* m_desk = nullptr;
    QScrollArea* m_scroll = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
};

} // namespace hustmcl::ui
