#pragma once

#include "hustmcl/application/content_market.h"
#include "page_scaffold.h"

#include <QPointer>
#include <QString>

class QBoxLayout;
class QComboBox;
class QFrame;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace hustmcl::ui {

class PrimaryActionBar;

// Modrinth single-source market page. It renders ContentSnapshot values and
// forwards explicit user intents to ContentController; it never performs
// network, file or compatibility work itself.
class ContentMarketPage : public PageScaffold {
    Q_OBJECT

public:
    explicit ContentMarketPage(application::ContentController* controller = nullptr,
                               QWidget* parent = nullptr);

    void setController(application::ContentController* controller);
    application::ContentController* controller() const;
    void setSnapshot(const application::ContentSnapshot& snapshot);
    void focusPrimaryAction();

signals:
    // Emitted only when the application snapshot says the download completed.
    // MainWindow owns opening the folder and checking that it still exists.
    void openDownloadFolderRequested(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateResponsiveLayout();
    void updateSearchLayout();
    void updateSearchState();
    void updateResults();
    void updateProject();
    void updateVersions();
    void updateDependency();
    void updateActionBar();
    void submitSearch(int offset);
    void selectVersion(const QString& versionId);
    const core::ContentVersion* selectedVersion() const;
    QString appliedFilterText() const;
    QString errorText(application::ContentError error) const;
    QString errorCode(application::ContentError error) const;
    void handleSecondary();

    QPointer<application::ContentController> m_controller;
    application::ContentSnapshot m_snapshot;    QString m_selectedVersionId;
    QString m_lastProjectId;
    bool m_initialized = false;

    QWidget* m_content = nullptr;
    QBoxLayout* m_body = nullptr;
    QGridLayout* m_searchGrid = nullptr;
    QWidget* m_resultsPane = nullptr;
    QWidget* m_detailPane = nullptr;
    QFrame* m_bodyDivider = nullptr;
    QLineEdit* m_searchText = nullptr;
    QLineEdit* m_gameVersion = nullptr;
    QComboBox* m_loader = nullptr;
    QPushButton* m_searchButton = nullptr;
    QLabel* m_resultsSummary = nullptr;
    QListWidget* m_results = nullptr;
    QPushButton* m_previousPage = nullptr;
    QPushButton* m_nextPage = nullptr;
    QLabel* m_pageLabel = nullptr;
    QLabel* m_projectTitle = nullptr;
    QLabel* m_projectDescription = nullptr;
    QLabel* m_projectMeta = nullptr;
    QLabel* m_filterLabel = nullptr;
    QLabel* m_versionsTitle = nullptr;
    QListWidget* m_versions = nullptr;
    QLabel* m_dependency = nullptr;
    QLabel* m_status = nullptr;
    QFrame* m_errorNotice = nullptr;
    QLabel* m_errorLabel = nullptr;
    QLabel* m_note = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
};

} // namespace hustmcl::ui
