#include "main_window.h"

#include "checks_page.h"
#include "content_market_page.h"
#include "copy.h"
#include "hustmcl/application/content_market.h"
#include "hustmcl/application/frontend_gateway.h"
#include "hustmcl/application/launcher_settings.h"
#include "hustmcl/application/modpack_import.h"
#include "home_page.h"
#include "install_page.h"
#include "logs_page.h"
#include "pixel_title_bar.h"
#include "settings_page.h"
#include "sidebar.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

namespace hustmcl::app {

namespace {
constexpr int kResizeMargin = 6;
}

MainWindow::MainWindow(application::FrontendGateway* gateway, Mode mode, QWidget* parent)
    : QMainWindow(parent)
    , m_gateway(gateway)
    , m_mode(mode)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint
                   | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint
                   | Qt::WindowCloseButtonHint);
    const QString title = m_mode == Mode::Real ? QStringLiteral("HustMCL")
                                               : QStringLiteral("HustMCL 界面演示");
    setWindowTitle(title);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMinimumSize(840, 460);
    resize(1120, 720);

    m_root = new QWidget(this);
    m_root->setObjectName(QStringLiteral("Root"));
    m_root->setAttribute(Qt::WA_Hover, true);
    m_root->installEventFilter(this);

    auto* outerLayout = new QVBoxLayout(m_root);
    outerLayout->setContentsMargins(2, 2, 2, 2);
    outerLayout->setSpacing(0);

    m_titleBar = new ui::PixelTitleBar(m_root);
    m_titleBar->setTitleText(
        title,
        m_mode == Mode::Real ? QStringLiteral("HUSTer的启动器")
                             : QStringLiteral("HUSTer的启动器 · 界面演示"));
    outerLayout->addWidget(m_titleBar, 0);

    auto* body = new QWidget(m_root);
    auto* layout = new QHBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_sidebar = new ui::Sidebar(body);
    m_sidebar->setFixedWidth(220);
    m_sidebar->setRealMode(m_mode == Mode::Real);
    layout->addWidget(m_sidebar, 0);

    m_stack = new QStackedWidget(body);
    m_home = new ui::HomePage(m_stack);
    if (m_mode == Mode::Real) {
        m_checks = new ui::ChecksPage(m_stack);
    } else {
        m_install = new ui::InstallPage(m_stack);
    }
    m_settings = new ui::SettingsPage(m_stack);
    m_logs = new ui::LogsPage(m_stack);
    m_market = new ui::ContentMarketPage(nullptr, m_stack);
    m_stack->addWidget(m_home);
    m_stack->addWidget(m_mode == Mode::Real ? static_cast<QWidget*>(m_checks)
                                            : static_cast<QWidget*>(m_install));
    m_stack->addWidget(m_settings);
    m_stack->addWidget(m_logs);
    m_stack->addWidget(m_market);
    layout->addWidget(m_stack, 1);
    outerLayout->addWidget(body, 1);
    setCentralWidget(m_root);

    // Theme mode persistence (UI-only QSettings) + zero-polling boundary timer
    m_themeTimer = new QTimer(this);
    m_themeTimer->setSingleShot(true);
    connect(m_themeTimer, &QTimer::timeout, this, [this]() {
        applyResolvedTheme(QTime::currentTime());
    });

    connect(m_titleBar, &ui::PixelTitleBar::themeCycleRequested,
            this, &MainWindow::cycleThemeMode);
    connect(m_titleBar, &ui::PixelTitleBar::minimizeRequested,
            this, &QWidget::showMinimized);
    connect(m_titleBar, &ui::PixelTitleBar::maximizeToggleRequested, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_titleBar, &ui::PixelTitleBar::closeRequested,
            this, &QWidget::close);

    connect(m_sidebar, &ui::Sidebar::pageActivated, this, [this](int index) {
        if (index == 1) {
            setCurrentPage(m_mode == Mode::Real ? Page::InstanceChecks : Page::Install);
        } else if (index == 2) {
            setCurrentPage(Page::Settings);
        } else if (index == 3) {
            setCurrentPage(Page::Logs);
        } else if (index == 4) {
            setCurrentPage(Page::Market);
        } else {
            setCurrentPage(Page::Home);
        }
    });
    connect(m_sidebar, &ui::Sidebar::unimplementedFeatureRequested,
            this, &MainWindow::showDemoMessage);
    connect(m_sidebar, &ui::Sidebar::accountSettingsRequested, this, [this]() {
        setCurrentPage(Page::Settings);
    });

    connect(m_home, &ui::HomePage::primaryActionRequested, this, [this]() {
        if (m_mode == Mode::Demo) {
            m_gateway->requestPrimaryAction();
            showDemoMessage(ui::copy::primaryActionText(
                m_gateway->homeSnapshot().primaryAction));
            return;
        }
        m_home->setPrimaryRequestPending(true);
        m_gateway->requestPrimaryAction();
    });
    connect(m_home, &ui::HomePage::installRequested, this, [this]() {
        if (m_mode == Mode::Demo) {
            setCurrentPage(Page::Install);
            return;
        }
        m_home->focusVanillaInstaller();
    });
    connect(m_home, &ui::HomePage::settingsRequested, this, [this]() {
        if (m_mode == Mode::Real) {
            setCurrentPage(Page::Settings);
        } else {
            showDemoMessage(QStringLiteral("设置页"));
        }
    });
    connect(m_home, &ui::HomePage::logsRequested, this, [this]() {
        if (m_mode == Mode::Real) {
            setCurrentPage(Page::Logs);
        } else {
            showDemoMessage(QStringLiteral("日志与错误页"));
        }
    });
    connect(m_home, &ui::HomePage::unimplementedFeatureRequested,
            this, &MainWindow::showDemoMessage);
    connect(m_home, &ui::HomePage::cancelRequested,
            this, [this](const QString& taskId) {
        m_gateway->requestCancel(taskId);
    });
    connect(m_home, &ui::HomePage::modpackImportRequested,
            this, [this](const QString& localPath) {
        if (m_modpackController) {
            m_modpackController->importFile(localPath);
        }
    });
    connect(m_home, &ui::HomePage::modpackCancelRequested,
            this, [this](const QString& taskId) {
        if (m_modpackController) {
            m_modpackController->cancel(taskId);
        }
    });
    connect(m_home, &ui::HomePage::modpackRetryRequested,
            this, [this](const QString& taskId) {
        if (m_modpackController) {
            m_modpackController->retry(taskId);
        }
    });
    connect(m_home, &ui::HomePage::modpackInstanceSelected,
            this, [this](const QString& instanceId) {
        if (m_modpackController) {
            m_modpackController->selectInstance(instanceId);
        }
    });
    connect(m_home, &ui::HomePage::vanillaInstallRequested,
            this, [this](const QString& version) {
        if (m_modpackController) {
            m_modpackController->installVanilla(version);
        }
    });
    connect(m_home, &ui::HomePage::vanillaVersionsReloadRequested,
            this, [this]() {
        if (m_modpackController) {
            m_modpackController->loadVanillaVersions();
        }
    });
    if (m_install) {
        connect(m_install, &ui::InstallPage::cancelRequested,
                this, [this](const QString& taskId) {
            m_gateway->requestCancel(taskId);
        });
        connect(m_install, &ui::InstallPage::retryRequested,
                this, [this](const QString& taskId) {
            m_gateway->requestRetry(taskId);
        });
    }
    if (m_checks) {
        connect(m_checks, &ui::ChecksPage::cancelRequested,
                this, [this](const QString& taskId) {
            m_gateway->requestCancel(taskId);
        });
        connect(m_checks, &ui::ChecksPage::retryRequested,
                this, [this](const QString& taskId) {
            m_gateway->requestRetry(taskId);
        });
    }
    connect(m_settings, &ui::SettingsPage::themeModeRequested,
            this, [this](ui::theme::ThemeMode mode) {
        setThemeMode(mode, QTime::currentTime(), true);
    });
    connect(m_settings, &ui::SettingsPage::saveSettingsRequested,
            this, [this](const QString& name, int memoryMb) {
        if (!m_homeSnapshot.offlineMode) {
            return;
        }
        if (m_homeSnapshot.sessionActive) {
            m_settings->setSaveResult(
                false, QStringLiteral("游戏运行中不能修改离线玩家名，请先退出游戏。"));
            m_settings->setMemorySaveResult(
                false, QStringLiteral("游戏运行中不能修改内存上限，请先退出游戏。"));
            return;
        }
        const bool nameAccepted = m_gateway->setPlayerName(name);
        if (nameAccepted) {
            m_settings->setSaveResult(
                true,
                QStringLiteral("离线玩家名已保存，下次启动游戏时生效。"));
        } else {
            m_settings->setSaveResult(
                false,
                QStringLiteral("玩家名无效：仅支持 1–16 位英文字母、数字或下划线，"
                               "不能包含中文或空格。若格式正确，请检查数据目录写入权限。"));
        }
        const bool memAccepted = m_gateway->setMemoryMb(memoryMb);
        if (memAccepted) {
            m_settings->setMemorySaveResult(
                true,
                QStringLiteral("内存上限已保存（%1 MB），下次启动游戏时生效。").arg(memoryMb));
        } else {
            m_settings->setMemorySaveResult(
                false,
                QStringLiteral("内存需在 %1–%2 MB 之间；若范围正确请检查数据目录写入权限。")
                    .arg(application::kMinMemoryMb)
                    .arg(application::kMaxMemoryMb));
        }
        if (nameAccepted || memAccepted) {
            refreshFromGateway();
        }
    });
    connect(m_settings, &ui::SettingsPage::saveMemoryRequested,
            this, [this](int memoryMb) {
        if (m_homeSnapshot.sessionActive) {
            m_settings->setMemorySaveResult(
                false, QStringLiteral("游戏运行中不能修改内存上限，请先退出游戏。"));
            return;
        }
        const bool accepted = m_gateway->setMemoryMb(memoryMb);
        if (accepted) {
            m_settings->setMemorySaveResult(
                true,
                QStringLiteral("内存上限已保存（%1 MB），下次启动游戏时生效。").arg(memoryMb));
            refreshFromGateway();
        } else {
            m_settings->setMemorySaveResult(
                false,
                QStringLiteral("内存需在 %1–%2 MB 之间；若范围正确请检查数据目录写入权限。")
                    .arg(application::kMinMemoryMb)
                    .arg(application::kMaxMemoryMb));
        }
    });
    connect(m_settings, &ui::SettingsPage::offlineModeRequested, this, [this](bool offline) {
        if (!m_homeSnapshot.canChangeLoginMode) return;
        const bool accepted = m_gateway->setOfflineMode(offline);
        refreshFromGateway();
        m_settings->setModeChangeResult(accepted);
    });
    connect(m_settings, &ui::SettingsPage::signInRequested, this, [this]() {
        if (m_homeSnapshot.offlineMode || m_homeSnapshot.sessionActive
            || m_homeSnapshot.account.busy || !m_homeSnapshot.account.canSignIn) {
            return;
        }
        m_gateway->requestSignIn();
    });
    connect(m_settings, &ui::SettingsPage::cancelSignInRequested, this, [this]() {
        if (m_homeSnapshot.offlineMode || m_homeSnapshot.sessionActive
            || !m_homeSnapshot.account.busy) {
            return;
        }
        m_gateway->requestCancelSignIn();
    });
    connect(m_settings, &ui::SettingsPage::signOutRequested, this, [this]() {
        if (m_homeSnapshot.offlineMode || m_homeSnapshot.sessionActive
            || m_homeSnapshot.account.busy || !m_homeSnapshot.account.canSignOut) {
            return;
        }
        m_gateway->requestSignOut();
    });
    connect(m_logs, &ui::LogsPage::openLogFolderRequested, this, [this]() {
        if (m_homeSnapshot.logPath.isEmpty()) {
            return;
        }
        openFolder(QFileInfo(m_homeSnapshot.logPath).absolutePath(),
                   QStringLiteral("还没有日志"),
                   QStringLiteral("启动游戏并生成运行日志后，才能打开日志文件夹。"));
    });
    connect(m_logs, &ui::LogsPage::openSavesFolderRequested, this, [this]() {
        if (m_homeSnapshot.dataPath.isEmpty()) {
            return;
        }
        openFolder(QDir(m_homeSnapshot.dataPath).filePath(QStringLiteral("game/saves")),
                   QStringLiteral("还没有存档文件夹"),
                   QStringLiteral("启动游戏并创建世界后，存档文件夹才会生成。"));
    });
    connect(m_market, &ui::ContentMarketPage::openDownloadFolderRequested,
            this, [this](const QString& filePath) {
        if (filePath.isEmpty()) {
            return;
        }
        const QFileInfo file(filePath);
        if (!file.isFile() || !file.absoluteDir().exists()) {
            QMessageBox::information(
                this, QStringLiteral("下载文件不可用"),
                QStringLiteral("下载文件已不在原位置，请重新下载后再打开文件夹。"));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(file.absolutePath()));
    });

    connect(m_gateway, &application::FrontendGateway::homeChanged,
            this, &MainWindow::applyHomeSnapshot);
    connect(m_gateway, &application::FrontendGateway::installChanged,
            this, &MainWindow::applyInstallSnapshot);

    const QSettings uiPrefs(QStringLiteral("HustMCL"), QStringLiteral("HustMCL"));
    const ui::theme::ThemeMode initialMode = ui::theme::themeModeFromKey(
        uiPrefs.value(QStringLiteral("ui/themeMode"), QStringLiteral("auto")).toString());
    setThemeMode(initialMode, QTime::currentTime(), false);

    refreshFromGateway();
    setCurrentPage(Page::Home);
    fitToScreen();
    updateSidebarForSize();
}

MainWindow::Mode MainWindow::mode() const
{
    return m_mode;
}

MainWindow::Page MainWindow::currentPage() const
{
    const int index = m_stack->currentIndex();
    if (index == 4) {
        return Page::Market;
    }
    if (index == 3) {
        return Page::Logs;
    }
    if (index == 2) {
        return Page::Settings;
    }
    if (index == 1) {
        return m_mode == Mode::Real ? Page::InstanceChecks : Page::Install;
    }
    return Page::Home;
}

void MainWindow::setCurrentPage(Page page)
{
    int index = 0;
    switch (page) {
    case Page::Home:
        index = 0;
        break;
    case Page::Install:
        index = 1;
        break;
    case Page::Settings:
        index = 2;
        break;
    case Page::Logs:
        index = 3;
        break;
    case Page::Market:
        index = 4;
        break;
    }
    if (m_mode == Mode::Demo && index > 1) {
        return;
    }
    if (index == 4 && !m_contentController) {
        return;
    }
    m_stack->setCurrentIndex(index);
    m_sidebar->setCurrentPage(index);
    m_sidebar->setArtworkVisible(index == 1 && m_decorationsVisible);
    focusCurrentPage();
}

void MainWindow::setDecorationsVisible(bool visible)
{
    m_decorationsVisible = visible;
    m_home->setDecorationsVisible(visible);
    if (m_install) {
        m_install->setDecorationsVisible(visible);
    }
    if (m_checks) {
        m_checks->setDecorationsVisible(visible);
    }
    m_settings->setDecorationsVisible(visible);
    m_logs->setDecorationsVisible(visible);
    m_market->setDecorationsVisible(visible);
    m_sidebar->setArtworkVisible(visible && currentPage() == Page::Install);
}

void MainWindow::refreshFromGateway()
{
    applyHomeSnapshot(m_gateway->homeSnapshot());
    applyInstallSnapshot(m_gateway->installSnapshot());
}

void MainWindow::setContentController(application::ContentController* controller)
{
    if (m_contentController == controller) {
        return;
    }
    m_contentController = controller;
    m_market->setController(controller);
    m_sidebar->setMarketAvailable(controller != nullptr);
    if (!controller && currentPage() == Page::Market) {
        setCurrentPage(Page::Home);
    }
}

void MainWindow::setModpackController(application::ModpackController* controller)
{
    if (m_modpackController == controller) {
        return;
    }
    if (m_modpackController) {
        disconnect(m_modpackController, nullptr, this, nullptr);
    }
    m_modpackController = controller;
    if (!controller) {
        m_home->setModpackAvailable(false);
        return;
    }
    connect(controller, &application::ModpackController::changed,
            this, [this](const application::ModpackSnapshot& snapshot) {
        m_home->setModpackSnapshot(snapshot);
    });
    connect(controller, &QObject::destroyed, this, [this, controller]() {
        if (m_modpackController != controller) {
            return;
        }
        m_modpackController = nullptr;
        m_home->setModpackAvailable(false);
    });
    m_home->setModpackAvailable(true);
    m_home->setModpackSnapshot(controller->snapshot());
}

ui::theme::ThemeMode MainWindow::themeMode() const
{
    return m_themeMode;
}

ui::theme::ResolvedTheme MainWindow::resolvedTheme() const
{
    return m_resolvedTheme;
}

void MainWindow::setThemeMode(ui::theme::ThemeMode mode, const QTime& localTime, bool persist)
{
    m_themeMode = mode;
    if (persist) {
        QSettings uiPrefs(QStringLiteral("HustMCL"), QStringLiteral("HustMCL"));
        uiPrefs.setValue(QStringLiteral("ui/themeMode"), ui::theme::themeModeKey(mode));
    }
    applyResolvedTheme(localTime);
}

void MainWindow::cycleThemeMode()
{
    using ui::theme::ThemeMode;
    ThemeMode next = ThemeMode::AutoByTime;
    if (m_themeMode == ThemeMode::AutoByTime) {
        next = ThemeMode::Dark;
    } else if (m_themeMode == ThemeMode::Dark) {
        next = ThemeMode::Light;
    } else {
        next = ThemeMode::AutoByTime;
    }
    setThemeMode(next, QTime::currentTime(), true);
}

void MainWindow::applyResolvedTheme(const QTime& localTime)
{
    const ui::theme::ResolvedTheme resolved = ui::theme::resolveTheme(m_themeMode, localTime);
    m_resolvedTheme = resolved;
    ui::theme::setActiveTheme(resolved);
    if (qApp) {
        qApp->setStyleSheet(ui::theme::styleSheet(resolved));
    }
    if (m_titleBar) {
        m_titleBar->setThemeState(m_themeMode, resolved);
    }
    if (m_sidebar) {
        m_sidebar->refreshTheme();
    }
    if (m_home) {
        m_home->refreshTheme(localTime);
    }
    if (m_settings) {
        m_settings->setThemeState(m_themeMode, resolved);
    }
    scheduleThemeBoundaryCheck(localTime);
}

void MainWindow::scheduleThemeBoundaryCheck(const QTime& localTime)
{
    if (!m_themeTimer) {
        return;
    }
    // Single-shot timer fires only on day/night & sky-phase boundary transitions.
    m_themeTimer->start(ui::theme::msecsUntilNextBoundary(localTime));
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateSidebarForSize();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(0, this, [this]() { focusCurrentPage(); });
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_mode == Mode::Real && m_sessionActive) {
        // Keep the event loop, controller and process owner alive. The window
        // object is hidden, not deleted, so late process events have a valid
        // receiver and closing the launcher never kills the game.
        event->accept();
        hide();
        return;
    }
    QMainWindow::closeEvent(event);
    if (m_mode == Mode::Real) {
        emit quitRequested();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_titleBar) {
        m_titleBar->setMaximized(isMaximized());
    }
}

Qt::Edges MainWindow::resizeEdgesForPos(const QPoint& pos) const
{
    if (isMaximized() || !m_root) {
        return {};
    }
    Qt::Edges edges;
    if (pos.x() <= kResizeMargin) {
        edges |= Qt::LeftEdge;
    } else if (pos.x() >= m_root->width() - kResizeMargin) {
        edges |= Qt::RightEdge;
    }
    if (pos.y() <= kResizeMargin) {
        edges |= Qt::TopEdge;
    } else if (pos.y() >= m_root->height() - kResizeMargin) {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_root) {
        if (event->type() == QEvent::HoverMove) {
            const auto* hover = static_cast<QHoverEvent*>(event);
            const Qt::Edges edges = resizeEdgesForPos(hover->position().toPoint());
            if ((edges == (Qt::LeftEdge | Qt::TopEdge))
                || (edges == (Qt::RightEdge | Qt::BottomEdge))) {
                m_root->setCursor(Qt::SizeFDiagCursor);
            } else if ((edges == (Qt::RightEdge | Qt::TopEdge))
                       || (edges == (Qt::LeftEdge | Qt::BottomEdge))) {
                m_root->setCursor(Qt::SizeBDiagCursor);
            } else if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
                m_root->setCursor(Qt::SizeHorCursor);
            } else if (edges & (Qt::TopEdge | Qt::BottomEdge)) {
                m_root->setCursor(Qt::SizeVerCursor);
            } else {
                m_root->unsetCursor();
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                const Qt::Edges edges = resizeEdgesForPos(mouse->position().toPoint());
                if (edges != Qt::Edges() && windowHandle()
                    && windowHandle()->startSystemResize(edges)) {
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

ui::HomePage* MainWindow::homePage() const
{
    return m_home;
}

ui::InstallPage* MainWindow::installPage() const
{
    return m_install;
}

ui::ChecksPage* MainWindow::checksPage() const
{
    return m_checks;
}

ui::SettingsPage* MainWindow::settingsPage() const
{
    return m_settings;
}

ui::LogsPage* MainWindow::logsPage() const
{
    return m_logs;
}

ui::ContentMarketPage* MainWindow::contentMarketPage() const
{
    return m_market;
}

ui::Sidebar* MainWindow::sidebar() const
{
    return m_sidebar;
}

ui::PixelTitleBar* MainWindow::titleBar() const
{
    return m_titleBar;
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
    const int sidebarWidth = width() < 960 ? 204 : 220;
    if (m_sidebar->width() != sidebarWidth) {
        m_sidebar->setFixedWidth(sidebarWidth);
    }
    m_sidebar->setCompact(height() < 680);
    if (m_titleBar) {
        m_titleBar->setCompact(width() < 940);
    }
}

void MainWindow::showDemoMessage(const QString& action)
{
    // Real mode must never show sample-data dialogs. Keep this path exclusively
    // for explicit --scenario runs.
    if (m_mode != Mode::Demo) {
        return;
    }
    const QString heading = action.isEmpty()
        ? QStringLiteral("界面演示")
        : QStringLiteral("界面演示 · %1").arg(action);
    QMessageBox::information(
        this,
        heading,
        QStringLiteral("当前窗口只演示页面状态与交互反馈，不执行真实账号、下载或游戏启动操作。"));
}

void MainWindow::applyHomeSnapshot(const application::HomeSnapshot& snapshot)
{
    const bool wasActive = m_sessionActive;
    m_sessionActive = snapshot.sessionActive;
    m_homeSnapshot = snapshot;
    m_home->setSnapshot(snapshot);
    m_home->setTaskSnapshot(m_installSnapshot.task);
    m_sidebar->setAccount(snapshot.account, snapshot.offlineMode);
    m_settings->setSnapshot(snapshot);
    if (m_checks) {
        m_checks->setSnapshot(snapshot, m_installSnapshot.task);
    }
    m_logs->setSnapshot(snapshot, m_installSnapshot.task);

    if (m_mode == Mode::Real && wasActive && !snapshot.sessionActive && !isVisible()) {
        emit quitRequested();
    }
}

void MainWindow::applyInstallSnapshot(const application::InstallSnapshot& snapshot)
{
    m_installSnapshot = snapshot;
    if (m_install) {
        m_install->setSnapshot(snapshot);
    }
    m_home->setTaskSnapshot(snapshot.task);
    if (m_checks) {
        m_checks->setSnapshot(m_homeSnapshot, snapshot.task);
    }
    m_logs->setSnapshot(m_homeSnapshot, snapshot.task);
}

void MainWindow::focusCurrentPage()
{
    switch (currentPage()) {
    case Page::Home:
        m_home->focusPrimaryAction();
        break;
    case Page::Install:
        if (m_install) {
            m_install->focusPrimaryAction();
        } else if (m_checks) {
            m_checks->focusPrimaryAction();
        }
        break;
    case Page::Settings:
        m_settings->focusNameField();
        break;
    case Page::Logs:
        m_logs->focusPrimaryAction();
        break;
    case Page::Market:
        m_market->focusPrimaryAction();
        break;
    }
}

void MainWindow::openFolder(const QString& path, const QString& missingTitle,
                            const QString& missingText)
{
    if (path.isEmpty()) {
        return;
    }
    const QString cleanPath = QDir::cleanPath(path);
    if (!QFileInfo(cleanPath).isDir()) {
        QMessageBox::information(this, missingTitle, missingText);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(cleanPath));
}

} // namespace hustmcl::app
