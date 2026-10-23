#pragma once

#include <QFrame>

class QLabel;
class QPushButton;

namespace hustmcl::ui {

class PrimaryActionBar : public QFrame {
    Q_OBJECT

public:
    explicit PrimaryActionBar(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setNote(const QString& note);
    void setPrimary(const QString& text, bool enabled, const QString& tooltip = {});
    void setSecondary(const QString& text, bool visible, bool enabled = true,
                      const QString& tooltip = {}, bool danger = false);
    void setPrimaryVisible(bool visible);
    void setSecondaryVisible(bool visible);
    void setErrorState(bool error);

    QPushButton* primaryButton() const;
    QPushButton* secondaryButton() const;

signals:
    void primaryClicked();
    void secondaryClicked();

private:
    void repolish(QWidget* widget);

    QLabel* m_title = nullptr;
    QLabel* m_note = nullptr;
    QPushButton* m_primary = nullptr;
    QPushButton* m_secondary = nullptr;
};

} // namespace hustmcl::ui
