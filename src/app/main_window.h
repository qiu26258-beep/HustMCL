#pragma once

#include <QMainWindow>

namespace hustmcl::application {
class FrontendGateway;
}

class QStackedWidget;

namespace hustmcl::ui {
class HomePage;
class InstallPage;
class Sidebar;
}

namespace hustmcl::app {

class MainWindow : public QMainWindow {
public:
    enum class Page {
        Home = 0,
        Install = 1,
    };

    explicit MainWindow(application::FrontendGateway* gateway, QWidget* parent = nullptr);

    Page currentPage() const;
    void setCurrentPage(Page page);
    void setDecorationsVisible(bool visible);
    void refreshFromGateway();

    ui::HomePage* homePage() const;
    ui::InstallPage* installPage() const;
    ui::Sidebar* sidebar() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void fitToScreen();
    void updateSidebarForSize();
    void showDemoMessage(const QString& action);

    application::FrontendGateway* m_gateway = nullptr;
    ui::Sidebar* m_sidebar = nullptr;
    QStackedWidget* m_stack = nullptr;
    ui::HomePage* m_home = nullptr;
    ui::InstallPage* m_install = nullptr;
    bool m_decorationsVisible = true;
};

} // namespace hustmcl::app
