#pragma once

#include <QWidget>

class QLabel;

namespace hustmcl::ui {

class LeafMotif;

class PageHeader : public QWidget {
public:
    explicit PageHeader(QWidget* parent = nullptr);

    void setContent(const QString& title, const QString& subtitle, const QString& trailing);
    void setTrailingText(const QString& text);
    void setDecorationsVisible(bool visible);

private:
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_trailing = nullptr;
    LeafMotif* m_leaf = nullptr;
};

} // namespace hustmcl::ui
