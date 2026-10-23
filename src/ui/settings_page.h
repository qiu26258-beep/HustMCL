#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"

class QLabel;
class QLineEdit;

namespace hustmcl::ui {

class ElidedLabel;
class PrimaryActionBar;
class SummaryRow;

// Real-mode settings page. It collects the offline identity and shows the
// read-only application snapshot; all validation and persistence stay in the
// application layer behind setPlayerName().
class SettingsPage : public PageScaffold {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    void setSnapshot(const application::HomeSnapshot& home);
    void setSaveResult(bool accepted, const QString& message);
    void focusNameField();

signals:
    void saveNameRequested(const QString& name);

private:
    void applyEnabledState();
    void clearFieldError();

    QLineEdit* m_nameInput = nullptr;
    QLabel* m_identityNote = nullptr;
    QLabel* m_fieldError = nullptr;
    QLabel* m_savedNote = nullptr;
    SummaryRow* m_memoryRow = nullptr;
    SummaryRow* m_versionRow = nullptr;
    SummaryRow* m_javaRow = nullptr;
    ElidedLabel* m_dataPath = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
    application::HomeSnapshot m_home;
    bool m_edited = false;
    bool m_sessionActive = false;
};

} // namespace hustmcl::ui
