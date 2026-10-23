#pragma once

#include <QWidget>

namespace hustmcl::ui {

class LeafMotif : public QWidget {
public:
    explicit LeafMotif(QWidget* parent = nullptr);

    void setDecorationVisible(bool visible);
    bool decorationVisible() const;

    QSize sizeHint() const override;

private:
    void paintEvent(QPaintEvent* event) override;

    bool m_decorationVisible = true;
};

} // namespace hustmcl::ui
