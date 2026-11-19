#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"
#include "theme.h"

#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace hustmcl::ui {

class ElidedLabel;
class PrimaryActionBar;
class SummaryRow;

// Real-mode settings page. Manages UI theme preference (auto/dark/light),
// login identity (Microsoft / Offline), and JVM memory allocation forwarded
// to FrontendGateway::setMemoryMb without duplicating backend rules.
class SettingsPage : public PageScaffold {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

    void setSnapshot(const application::HomeSnapshot& home);
    void setThemeState(theme::ThemeMode mode, theme::ResolvedTheme resolved);
    void setSaveResult(bool accepted, const QString& message);
    void setMemorySaveResult(bool accepted, const QString& message);
    void setModeChangeResult(bool accepted);
    void focusNameField();

signals:
    void saveNameRequested(const QString& name);
    void saveSettingsRequested(const QString& name, int memoryMb);
    void saveMemoryRequested(int memoryMb);
    void themeModeRequested(hustmcl::ui::theme::ThemeMode mode);
    void signInRequested();
    void signOutRequested();
    void cancelSignInRequested();
    void offlineModeRequested(bool offline);

private:
    void applyEnabledState();
    void applyAccountState();
    void updatePresetHighlights(int memoryMb);
    void clearFieldFeedback();

    QComboBox* m_themeMode = nullptr;
    QLabel* m_themeHint = nullptr;
    QComboBox* m_loginMode = nullptr;
    QLabel* m_modeError = nullptr;
    QWidget* m_offlineSection = nullptr;
    QWidget* m_accountSection = nullptr;
    QLineEdit* m_nameInput = nullptr;
    QLabel* m_identityNote = nullptr;
    QLabel* m_fieldError = nullptr;
    QLabel* m_savedNote = nullptr;
    QLabel* m_accountAvatar = nullptr;
    QLabel* m_accountNote = nullptr;
    QLabel* m_accountName = nullptr;
    QLabel* m_accountMeta = nullptr;
    QLabel* m_accountMessage = nullptr;
    QLabel* m_accountHint = nullptr;
    QPushButton* m_signInButton = nullptr;
    QPushButton* m_cancelSignInButton = nullptr;
    QPushButton* m_signOutButton = nullptr;
    QSpinBox* m_memoryInput = nullptr;
    QVector<QPushButton*> m_memoryPresets;
    QLabel* m_memoryError = nullptr;
    QLabel* m_memorySaved = nullptr;
    SummaryRow* m_versionRow = nullptr;
    SummaryRow* m_javaRow = nullptr;
    ElidedLabel* m_dataPath = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
    application::HomeSnapshot m_home;
    bool m_nameEdited = false;
    bool m_memoryEdited = false;
    bool m_sessionActive = false;
    bool m_offlineMode = true;
};

} // namespace hustmcl::ui
