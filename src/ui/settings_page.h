#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace hustmcl::ui {

class ElidedLabel;
class PrimaryActionBar;
class SummaryRow;

// Real-mode settings page. In offline mode it collects the local identity; in
// online mode it shows the account snapshot and forwards sign-in, cancel and
// sign-out intent. All validation, network work and persistence stay in the
// application layer.
class SettingsPage : public PageScaffold {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    void setSnapshot(const application::HomeSnapshot& home);
    void setSaveResult(bool accepted, const QString& message);
    void focusNameField();

signals:
    void saveNameRequested(const QString& name);
    void signInRequested();
    void signOutRequested();
    void cancelSignInRequested();

private:
    void applyEnabledState();
    void applyAccountState();
    void clearFieldError();

    QWidget* m_offlineSection = nullptr;
    QWidget* m_accountSection = nullptr;
    QLineEdit* m_nameInput = nullptr;
    QLabel* m_identityNote = nullptr;
    QLabel* m_fieldError = nullptr;
    QLabel* m_savedNote = nullptr;
    QLabel* m_accountNote = nullptr;
    QLabel* m_accountName = nullptr;
    QLabel* m_accountMeta = nullptr;
    QLabel* m_accountMessage = nullptr;
    QLabel* m_accountHint = nullptr;
    QPushButton* m_signInButton = nullptr;
    QPushButton* m_cancelSignInButton = nullptr;
    QPushButton* m_signOutButton = nullptr;
    SummaryRow* m_memoryRow = nullptr;
    SummaryRow* m_versionRow = nullptr;
    SummaryRow* m_javaRow = nullptr;
    ElidedLabel* m_dataPath = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
    application::HomeSnapshot m_home;
    bool m_edited = false;
    bool m_sessionActive = false;
    bool m_offlineMode = true;
};

} // namespace hustmcl::ui
