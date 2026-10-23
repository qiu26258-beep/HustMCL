#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QMainWindow>

namespace hustmcl::application {
class ContentController;
class FrontendGateway;
class ModpackController;
}

class QCloseEvent;
class QStackedWidget;

namespace hustmcl::ui {
class ChecksPage;
class ContentMarketPage;
class HomePage;
class InstallPage;
class LogsPage;
class SettingsPage;
class Sidebar;
}

namespace hustmcl::app {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class Mode {
        Demo,
        Real,
    };

    enum class Page {
        Home = 0,
        Install = 1,
        InstanceChecks = Install,
        Settings = 2,
        Logs = 3,
        Market = 4,
    };

    explicit MainWindow(application::FrontendGateway* gateway,
                        Mode mode = Mode::Real,
                        QWidget* parent = nullptr);

    Mode mode() const;
    Page currentPage() const;
    void setCurrentPage(Page page);
    void setDecorationsVisible(bool visible);
    void refreshFromGateway();
    void setContentController(application::ContentController* controller);
    void setModpackController(application::ModpackController* controller);

    ui::HomePage* homePage() const;
    ui::InstallPage* installPage() const;
    ui::ChecksPage* checksPage() const;
    ui::SettingsPage* settingsPage() const;
    ui::LogsPage* logsPage() const;
    ui::ContentMarketPage* contentMarketPage() const;
    ui::Sidebar* sidebar() const;

signals:
    // Emitted when the real launcher may leave its event loop. Closing the
    // window while a game is active never emits this until the game ends.
    void quitRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void fitToScreen();
    void updateSidebarForSize();
    void showDemoMessage(const QString& action);
    void applyHomeSnapshot(const application::HomeSnapshot& snapshot);
    void applyInstallSnapshot(const application::InstallSnapshot& snapshot);
    void focusCurrentPage();
    void openFolder(const QString& path, const QString& missingTitle,
                    const QString& missingText);

    application::FrontendGateway* m_gateway = nullptr;
    application::ContentController* m_contentController = nullptr;
    application::ModpackController* m_modpackController = nullptr;
    Mode m_mode = Mode::Real;
    ui::Sidebar* m_sidebar = nullptr;
    QStackedWidget* m_stack = nullptr;
    ui::HomePage* m_home = nullptr;
    ui::InstallPage* m_install = nullptr;
    ui::ChecksPage* m_checks = nullptr;
    ui::SettingsPage* m_settings = nullptr;
    ui::LogsPage* m_logs = nullptr;
    ui::ContentMarketPage* m_market = nullptr;
    application::HomeSnapshot m_homeSnapshot;
    application::InstallSnapshot m_installSnapshot;
    bool m_decorationsVisible = true;
    bool m_sessionActive = false;
};

} // namespace hustmcl::app
