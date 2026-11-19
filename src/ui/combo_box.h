#pragma once

#include <QComboBox>

namespace hustmcl::ui {

// QComboBox with a flat, theme-consistent arrow. The style sheet owns the
// field chrome; this tiny paint step replaces the native raised arrow button
// without touching selection, popup or signal behaviour.
class ComboBox : public QComboBox {
public:
    explicit ComboBox(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace hustmcl::ui
