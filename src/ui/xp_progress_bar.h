#pragma once

#include <QProgressBar>

namespace hustmcl::ui {

// Minecraft-inspired notched experience bar. Subclasses QProgressBar so value,
// range, accessibility and existing UI tests remain 100% compatible.
class XpProgressBar : public QProgressBar {
    Q_OBJECT

public:
    explicit XpProgressBar(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace hustmcl::ui
