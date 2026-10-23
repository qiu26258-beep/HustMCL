#pragma once

#include "hustmcl/application/frontend_snapshot.h"
#include "hustmcl/application/modpack_import.h"
#include "page_scaffold.h"

#include <QList>
#include <QUrl>
#include <QWidget>

class QBoxLayout;
class QComboBox;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QFrame;
class QLabel;
class QProgressBar;
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
    void setTaskSnapshot(const application::TaskSnapshot& task);
    // Presentation-only guard: the application snapshot remains the source of
    // truth, but a second click while the launch request is in flight is
    // ignored until the next snapshot arrives.
    void setPrimaryRequestPending(bool pending);
    void setModpackAvailable(bool available);
    void setModpackSnapshot(const application::ModpackSnapshot& snapshot);
    // Shared by the native drop event and UI tests; rejects non-local,
    // multiple and unsupported files before asking the application to import.
    void handleDroppedUrls(const QList<QUrl>& urls);
    void setDecorationsVisible(bool visible);
    void focusPrimaryAction();

signals:
    void primaryActionRequested();
    void installRequested();
    void settingsRequested();
    void logsRequested();
    void cancelRequested(const QString& taskId);
    void unimplementedFeatureRequested(const QString& feature);
    void modpackImportRequested(const QString& localPath);
    void modpackCancelRequested(const QString& taskId);
    void modpackRetryRequested(const QString& taskId);
    void modpackInstanceSelected(const QString& instanceId);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateResponsiveLayout();
    void applyActionBar();
    void updateModpackUi();
    void chooseImportFile();
    void requestImport(const QString& localPath);
    void showImportHint(const QString& text);
    bool importLocked() const;
    QString actionTitle(const application::HomeSnapshot& snapshot) const;

    application::HomeSnapshot m_snapshot;
    application::TaskSnapshot m_task;
    application::ModpackSnapshot m_modpackSnapshot;
    bool m_primaryPending = false;
    bool m_modpackAvailable = false;
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
    QWidget* m_modpackSection = nullptr;
    QFrame* m_modpackDropZone = nullptr;
    QPushButton* m_modpackPick = nullptr;
    QComboBox* m_modpackInstance = nullptr;
    QLabel* m_modpackPackName = nullptr;
    QLabel* m_modpackPhase = nullptr;
    QProgressBar* m_modpackProgress = nullptr;
    QLabel* m_modpackProgressText = nullptr;
    QLabel* m_modpackError = nullptr;
    QLabel* m_modpackHint = nullptr;
    QPushButton* m_modpackCancel = nullptr;
    QPushButton* m_modpackRetry = nullptr;
    QLabel* m_modpackNote = nullptr;
    CampusArtwork* m_artwork = nullptr;
    QLabel* m_artCaption = nullptr;
    QLabel* m_artSub = nullptr;
    QPushButton* m_settingsLink = nullptr;
    QPushButton* m_logsLink = nullptr;
    PrimaryActionBar* m_actionBar = nullptr;
};

} // namespace hustmcl::ui
