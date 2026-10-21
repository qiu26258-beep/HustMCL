#include "main_window.h"

#include "hustmcl/application/frontend_gateway.h"
#include "copy.h"
#include "home_page.h"
#include "install_page.h"
#include "sidebar.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>

namespace hustmcl::app {

MainWindow::MainWindow(application::FrontendGateway* gateway, QWidget* parent)
    : QMainWindow(parent)
    , m_gateway(gateway)
{
    setWindowTitle(QStringLiteral("HustMCL 界面演示"));
    setMinimumSize(840, 460);
    resize(1120, 720);

    auto* root = new QWidget(this);
    root->setObjectName(QStringLiteral("Root"));
    auto* layout = new QHBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_sidebar = new ui::Sidebar(root);
    m_sidebar->setFixedWidth(216);
    layout->addWidget(m_sidebar, 0);

    m_stack = new QStackedWidget(root);
    m_home = new ui::HomePage(m_stack);
    m_install = new ui::InstallPage(m_stack);
    m_stack->addWidget(m_home);
    m_stack->addWidget(m_install);
    layout->addWidget(m_stack, 1);
    setCentralWidget(root);

    connect(m_sidebar, &ui::Sidebar::pageActivated, this, [this](int index) {
        setCurrentPage(static_cast<Page>(index));
    });
    connect(m_sidebar, &ui::Sidebar::unimplementedFeatureRequested,
            this, &MainWindow::showDemoMessage);
    connect(m_home, &ui::HomePage::primaryActionRequested, this, [this]() {
        m_gateway->requestPrimaryAction();
        const QString action = ui::copy::primaryActionText(m_gateway->homeSnapshot().primaryAction);
        showDemoMessage(action);
    });
    connect(m_home, &ui::HomePage::installRequested, this, [this]() {
        setCurrentPage(Page::Install);
    });
    connect(m_home, &ui::HomePage::unimplementedFeatureRequested,
            this, &MainWindow::showDemoMessage);
    connect(m_install, &ui::InstallPage::cancelRequested,
            this, [this](const QString& taskId) {
        m_gateway->requestCancel(taskId);
    });
    connect(m_install, &ui::InstallPage::retryRequested,
            this, [this](const QString& taskId) {
        m_gateway->requestRetry(taskId);
    });
    connect(m_gateway, &application::FrontendGateway::homeChanged,
            this, [this](const application::HomeSnapshot& snapshot) {
        m_home->setSnapshot(snapshot);
        m_sidebar->setAccount(snapshot.account);
    });
    connect(m_gateway, &application::FrontendGateway::installChanged,
            this, [this](const application::InstallSnapshot& snapshot) {
        m_install->setSnapshot(snapshot);
    });

    refreshFromGateway();
    setCurrentPage(Page::Home);
    fitToScreen();
    updateSidebarForSize();
}

MainWindow::Page MainWindow::currentPage() const
{
    return m_stack->currentIndex() == 1 ? Page::Install : Page::Home;
}

void MainWindow::setCurrentPage(Page page)
{
    const int index = page == Page::Install ? 1 : 0;
    m_stack->setCurrentIndex(index);
    m_sidebar->setCurrentPage(index);
    m_sidebar->setArtworkVisible(index == 1 && m_decorationsVisible);
    if (index == 1) {
        m_install->focusPrimaryAction();
    } else {
        m_home->focusPrimaryAction();
    }
}

void MainWindow::setDecorationsVisible(bool visible)
{
    m_decorationsVisible = visible;
    m_home->setDecorationsVisible(visible);
    m_install->setDecorationsVisible(visible);
    m_sidebar->setArtworkVisible(visible && currentPage() == Page::Install);
}

void MainWindow::refreshFromGateway()
{
    const application::HomeSnapshot home = m_gateway->homeSnapshot();
    m_home->setSnapshot(home);
    m_sidebar->setAccount(home.account);
    m_install->setSnapshot(m_gateway->installSnapshot());
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateSidebarForSize();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        if (currentPage() == Page::Install) {
            m_install->focusPrimaryAction();
        } else {
            m_home->focusPrimaryAction();
        }
    });
}

ui::HomePage* MainWindow::homePage() const
{
    return m_home;
}

ui::InstallPage* MainWindow::installPage() const
{
    return m_install;
}

ui::Sidebar* MainWindow::sidebar() const
{
    return m_sidebar;
}

void MainWindow::fitToScreen()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }
    const QRect available = screen->availableGeometry();
    const int width = qBound(840, qMin(1120, available.width() - 40), 1120);
    const int height = qBound(460, qMin(720, available.height() - 40), 720);
    resize(width, height);
}

void MainWindow::updateSidebarForSize()
{
    if (!m_sidebar) {
        return;
    }
    const int sidebarWidth = width() < 960 ? 200 : 216;
    if (m_sidebar->width() != sidebarWidth) {
        m_sidebar->setFixedWidth(sidebarWidth);
    }
    m_sidebar->setCompact(height() < 680);
}

void MainWindow::showDemoMessage(const QString& action)
{
    const QString heading = action.isEmpty()
        ? QStringLiteral("界面演示")
        : QStringLiteral("界面演示 · %1").arg(action);
    QMessageBox::information(
        this,
        heading,
        QStringLiteral("真实登录、下载与游戏启动尚未接入。\n当前窗口只演示页面状态与交互反馈。"));
}

} // namespace hustmcl::app
