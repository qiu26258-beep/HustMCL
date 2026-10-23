#pragma once

#include <QWidget>

class QLabel;

namespace hustmcl::ui {

class StatusRow : public QWidget {
public:
    explicit StatusRow(QWidget* parent = nullptr);

    void setData(const QString& name, const QString& value, const QString& state, bool ok);
    void setState(bool ok);

private:
    void repolish();

    QLabel* m_check = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_value = nullptr;
    QLabel* m_state = nullptr;
};

} // namespace hustmcl::ui
