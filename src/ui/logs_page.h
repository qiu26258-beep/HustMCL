#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"

class QFrame;
class QLabel;
class QPushButton;

namespace hustmcl::ui {

class ElidedLabel;
class PrimaryActionBar;

// Real-mode logs page. It only presents snapshot paths and emits an intent;
// reading files and opening Explorer belong to the application entry point.
class LogsPage : public PageScaffold {
    Q_OBJECT

public:
    explicit LogsPage(QWidget* parent = nullptr);

    void setSnapshot(const application::HomeSnapshot& home,
                     const application::TaskSnapshot& task);
    void focusPrimaryAction();

signals:
    void openLogFolderRequested();
    void openSavesFolderRequested();

private:
    QLabel* m_statusTitle = nullptr;
    QLabel* m_statusDetail = nullptr;
    QLabel* m_identity = nullptr;
    QFrame* m_errorNotice = nullptr;
    QLabel* m_errorTitle = nullptr;
    QLabel* m_errorSummary = nullptr;
    QLabel* m_errorDetail = nullptr;
    QLabel* m_noError = nullptr;
    ElidedLabel* m_logPath = nullptr;
    ElidedLabel* m_dataPath = nullptr;
    QPushButton* m_openLogs = nullptr;
    QPushButton* m_openSaves = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
    application::HomeSnapshot m_home;
};

} // namespace hustmcl::ui
