#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"

class QBoxLayout;
class QFrame;
class QLabel;

namespace hustmcl::ui {

class ElidedLabel;
class PrimaryActionBar;
class StatusRow;
class SummaryRow;

// Real-mode page 02. It reports the last application-provided instance,
// environment and preparation state; it never reads files or starts a check.
class ChecksPage : public PageScaffold {
    Q_OBJECT

public:
    explicit ChecksPage(QWidget* parent = nullptr);

    void setSnapshot(const application::HomeSnapshot& home,
                     const application::TaskSnapshot& task);
    void focusPrimaryAction();

signals:
    void cancelRequested(const QString& taskId);
    void retryRequested(const QString& taskId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateResponsiveLayout();
    void updateActionBar();

    QWidget* m_content = nullptr;
    QBoxLayout* m_columns = nullptr;
    QWidget* m_left = nullptr;
    QWidget* m_right = nullptr;
    QFrame* m_divider = nullptr;
    StatusRow* m_identityRow = nullptr;
    StatusRow* m_javaRow = nullptr;
    StatusRow* m_instanceRow = nullptr;
    StatusRow* m_fileCheckRow = nullptr;
    QLabel* m_checkDetail = nullptr;
    QFrame* m_errorNotice = nullptr;
    QLabel* m_errorCode = nullptr;
    QLabel* m_errorSummary = nullptr;
    QLabel* m_errorDetail = nullptr;
    ElidedLabel* m_dataPath = nullptr;
    ElidedLabel* m_logPath = nullptr;
    SummaryRow* m_memoryRow = nullptr;
    SummaryRow* m_versionRow = nullptr;
    QLabel* m_note = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
    application::HomeSnapshot m_home;
    application::TaskSnapshot m_task;
};

} // namespace hustmcl::ui
