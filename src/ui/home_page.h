#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "page_scaffold.h"

#include <QWidget>

class QBoxLayout;
class QFrame;
class QLabel;
class QPushButton;

namespace hustmcl::ui {

class CampusArtwork;
class PrimaryActionBar;
class StatusRow;
class SummaryRow;

class HomePage : public PageScaffold {
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);

    void setSnapshot(const application::HomeSnapshot& snapshot);
    void setDecorationsVisible(bool visible);
    void focusPrimaryAction();

signals:
    void primaryActionRequested();
    void installRequested();
    void unimplementedFeatureRequested(const QString& feature);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateResponsiveLayout();
    QString actionTitle(const application::HomeSnapshot& snapshot) const;

    QWidget* m_content = nullptr;
    QBoxLayout* m_columns = nullptr;
    QWidget* m_left = nullptr;
    QWidget* m_right = nullptr;
    QFrame* m_divider = nullptr;
    QLabel* m_instanceName = nullptr;
    QLabel* m_instanceMeta = nullptr;
    StatusRow* m_accountRow = nullptr;
    StatusRow* m_javaRow = nullptr;
    StatusRow* m_instanceRow = nullptr;
    SummaryRow* m_memoryRow = nullptr;
    SummaryRow* m_launchRow = nullptr;
    QLabel* m_footnote = nullptr;
    CampusArtwork* m_artwork = nullptr;
    QLabel* m_artCaption = nullptr;
    QLabel* m_artSub = nullptr;
    QPushButton* m_settingsLink = nullptr;
    QPushButton* m_logsLink = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
};

} // namespace hustmcl::ui
