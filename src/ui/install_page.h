#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"

#include <QWidget>

class QBoxLayout;
class QFrame;
class QLabel;
class QPushButton;

namespace hustmcl::ui {

class ElidedLabel;
class PrimaryActionBar;
class StageList;
class SummaryRow;

class InstallPage : public PageScaffold {
    Q_OBJECT

public:
    explicit InstallPage(QWidget* parent = nullptr);

    void setSnapshot(const application::InstallSnapshot& snapshot);
    void focusPrimaryAction();

signals:
    void cancelRequested(const QString& taskId);
    void retryRequested(const QString& taskId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateResponsiveLayout();
    void updateActionBar();
    QString actionNote() const;

    QWidget* m_content = nullptr;
    QBoxLayout* m_columns = nullptr;
    QWidget* m_left = nullptr;
    QWidget* m_right = nullptr;
    QFrame* m_divider = nullptr;
    StageList* m_stageList = nullptr;
    QLabel* m_version = nullptr;
    QLabel* m_recommendedBadge = nullptr;
    QLabel* m_verifiedBadge = nullptr;
    QLabel* m_versionMeta = nullptr;
    ElidedLabel* m_path = nullptr;
    SummaryRow* m_requiredRow = nullptr;
    SummaryRow* m_freeRow = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
    application::InstallSnapshot m_snapshot;
};

} // namespace hustmcl::ui
