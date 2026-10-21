#pragma once

#include <QWidget>

class QLabel;

namespace hustmcl::ui {

class SummaryRow : public QWidget {
public:
    explicit SummaryRow(QWidget* parent = nullptr);

    void setData(const QString& label, const QString& value);

private:
    QLabel* m_label = nullptr;
    QLabel* m_value = nullptr;
};

} // namespace hustmcl::ui
