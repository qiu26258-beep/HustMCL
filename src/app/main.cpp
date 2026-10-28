#include "demo_controller.h"
#include "main_window.h"

#include "hustmcl/application/launch_preparation.h"
#include "hustmcl/application/offline_controller.h"
#include "hustmcl/services/java_probe.h"
#include "hustmcl/services/offline_session.h"
#include "hustmcl/services/json_settings_store.h"
#include "hustmcl/services/account_service.h"
#include "hustmcl/application/content_market.h"
#include "hustmcl/services/modrinth_content.h"
#include "hustmcl/services/content_download_store.h"
#include "hustmcl/services/file_acquirer.h"
#include "hustmcl/services/modpack_workspace.h"

#include "theme.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QStyleFactory>
#include <QTimer>

#include <cstdio>
#include <optional>

namespace {

void applyScaleFromArgs(int argc, char** argv)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == QStringLiteral("--scale")) {
            const QString value = QString::fromLocal8Bit(argv[i + 1]);
            bool ok = false;
            const double scale = value.toDouble(&ok);
            if (ok && scale >= 1.0 && scale <= 4.0) {
                qputenv("QT_SCALE_FACTOR", value.toLatin1());
            }
        }
    }
}

bool savePixmap(const QPixmap& pixmap, const QString& path)
{
    if (pixmap.isNull()) {
        std::fprintf(stderr, "screenshot is null\n");
        return false;
    }
    const QFileInfo info(path);
    const QString directory = info.absolutePath();
    if (!directory.isEmpty() && !QDir().mkpath(directory)) {
        std::fprintf(stderr, "cannot create screenshot directory\n");
        return false;
    }
    if (!pixmap.save(path, "PNG")) {
        std::fprintf(stderr, "cannot save screenshot: %s\n", path.toLocal8Bit().constData());
        return false;
    }
    std::printf("screenshot=%s size=%dx%d dpr=%.2f\n",
                path.toLocal8Bit().constData(),
                pixmap.width(),
                pixmap.height(),
                pixmap.devicePixelRatio());
    return true;
}

QString resolvePath(const QString& value, const QString& base)
{
    const QString path = QDir::isRelativePath(value) ? QDir(base).filePath(value) : value;
    return QDir::cleanPath(path);
}

} // namespace

int main(int argc, char** argv)
{
    applyScaleFromArgs(argc, argv);
    if (qEnvironmentVariable("QT_QPA_PLATFORM") == QStringLiteral("offscreen")
        && qEnvironmentVariableIsEmpty("QT_QPA_FONTDIR")) {
        // The basic offscreen font database needs an explicit Windows font
        // directory to render Chinese in repeatable CI/review captures.
        qputenv("QT_QPA_FONTDIR", "C:\\Windows\\Fonts");
    }
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("HustMCL"));
    QApplication::setApplicationDisplayName(QStringLiteral("HustMCL"));
#ifdef HUSTMCL_PUBLIC_RELEASE
    QApplication::setApplicationVersion(QStringLiteral("0.1.0-public-preview"));
#else
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
#endif
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setFont(hustmcl::ui::theme::baseFont());
    app.setStyleSheet(hustmcl::ui::theme::styleSheet());
    Q_INIT_RESOURCE(resources);

    const QDir base(QCoreApplication::applicationDirPath());
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("HustMCL Minecraft Java launcher. Demo data is available only "
                       "with an explicit --scenario."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption scenarioOption(
        QStringLiteral("scenario"),
        QStringLiteral("Explicit demo state: home-ready, install-unknown, install-known, "
                       "install-cancelling, install-cancelled, install-committing, "
                       "install-failed, install-long-path."),
        QStringLiteral("name"));
    QCommandLineOption dataRootOption(
        QStringLiteral("data-root"),
        QStringLiteral("Existing verified local game data root."),
        QStringLiteral("path"),
        base.filePath(QStringLiteral("data")));
    QCommandLineOption javaOption(
        QStringLiteral("java"),
        QStringLiteral("Java executable used for the real launch probe."),
        QStringLiteral("path"),
        base.filePath(QStringLiteral("runtime/bin/java.exe")));
    QCommandLineOption gameVersionOption(
        QStringLiteral("game-version"),
        QStringLiteral("Installed game version identifier."),
        QStringLiteral("id"),
        QStringLiteral("26.2"));
    QCommandLineOption usernameOption(
        QStringLiteral("username"),
        QStringLiteral("Offline player name (1-16 ASCII letters, digits or underscores)."),
        QStringLiteral("name"),
        QStringLiteral("HustPlayer"));
    QCommandLineOption offlineOption(QStringLiteral("offline"), QStringLiteral("Explicit local offline identity; does not use a Microsoft account."));
    QCommandLineOption pageOption(
        QStringLiteral("page"),
        QStringLiteral("Initial page: auto, home, checks, install, settings, logs or market."),
        QStringLiteral("page"),
        QStringLiteral("auto"));
    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Render the window client to a PNG and exit."),
        QStringLiteral("path"));
    QCommandLineOption nativeScreenshotOption(
        QStringLiteral("native-screenshot"),
        QStringLiteral("Capture the shown native window to a PNG and exit."),
        QStringLiteral("path"));
    QCommandLineOption windowSizeOption(
        QStringLiteral("window-size"),
        QStringLiteral("Logical client size, for example 1120x720."),
        QStringLiteral("WxH"));
    QCommandLineOption scaleOption(
        QStringLiteral("scale"),
        QStringLiteral("Qt device scale factor applied before QApplication."),
        QStringLiteral("factor"));
    QCommandLineOption noDecorationOption(
        QStringLiteral("no-decoration"),
        QStringLiteral("Hide campus artwork and leaf decoration."));
    parser.addOption(scenarioOption);
    parser.addOption(dataRootOption);
    parser.addOption(javaOption);
    parser.addOption(gameVersionOption);
    parser.addOption(usernameOption);
    parser.addOption(offlineOption);
    parser.addOption(pageOption);
    parser.addOption(screenshotOption);
    parser.addOption(nativeScreenshotOption);
    parser.addOption(windowSizeOption);
    parser.addOption(scaleOption);
    parser.addOption(noDecorationOption);
    parser.process(app);

    const bool demoMode = parser.isSet(scenarioOption);
    const QString pageValue = parser.value(pageOption).toLower();
    if (pageValue != QStringLiteral("auto") && pageValue != QStringLiteral("home")
        && pageValue != QStringLiteral("checks") && pageValue != QStringLiteral("install")
        && pageValue != QStringLiteral("settings") && pageValue != QStringLiteral("logs")
        && pageValue != QStringLiteral("market")) {
        std::fprintf(stderr, "unknown page\n");
        return 2;
    }
    if (parser.isSet(screenshotOption) && parser.isSet(nativeScreenshotOption)) {
        std::fprintf(stderr, "choose one screenshot mode\n");
        return 2;
    }

    const bool wantsScreenshot =
        parser.isSet(screenshotOption) || parser.isSet(nativeScreenshotOption);
    const bool noDecoration = parser.isSet(noDecorationOption);

    auto initialPage = [&](bool demo,
                           const std::optional<hustmcl::app::DemoScenario>& scenario) {
        using Page = hustmcl::app::MainWindow::Page;
        if (pageValue == QStringLiteral("home")) {
            return Page::Home;
        }
        if (pageValue == QStringLiteral("checks")) {
            return demo ? Page::Install : Page::InstanceChecks;
        }
        if (pageValue == QStringLiteral("install")) {
            return Page::Install;
        }
        if (pageValue == QStringLiteral("settings")) {
            return Page::Settings;
        }
        if (pageValue == QStringLiteral("logs")) {
            return Page::Logs;
        }
        if (pageValue == QStringLiteral("market")) {
            return demo ? Page::Home : Page::Market;
        }
        if (demo && scenario.has_value()) {
            switch (scenario.value()) {
            case hustmcl::app::DemoScenario::HomeReady:
                return Page::Home;
            case hustmcl::app::DemoScenario::InstallUnknown:
            case hustmcl::app::DemoScenario::InstallKnown:
            case hustmcl::app::DemoScenario::InstallCancelling:
            case hustmcl::app::DemoScenario::InstallCancelled:
            case hustmcl::app::DemoScenario::InstallCommitting:
            case hustmcl::app::DemoScenario::InstallFailed:
            case hustmcl::app::DemoScenario::InstallLongPath:
                return Page::Install;
            }
        }
        return Page::Home;
    };

    const auto runWindow = [&](hustmcl::application::FrontendGateway* gateway,
                               hustmcl::app::MainWindow::Mode mode,
                               std::optional<hustmcl::app::DemoScenario> scenario,
                               hustmcl::application::ContentController* content = nullptr,
                               hustmcl::application::ModpackController* modpacks = nullptr) {
        hustmcl::app::MainWindow window(gateway, mode);
        window.setContentController(content);
        window.setModpackController(modpacks);
        window.setDecorationsVisible(!noDecoration);
        window.setCurrentPage(initialPage(mode == hustmcl::app::MainWindow::Mode::Demo,
                                          scenario));
        QObject::connect(&window, &hustmcl::app::MainWindow::quitRequested,
                         &app, []() { QCoreApplication::quit(); });

        if (parser.isSet(windowSizeOption)) {
            const QStringList parts = parser.value(windowSizeOption).split(QLatin1Char('x'));
            bool okWidth = false;
            bool okHeight = false;
            const int width = parts.value(0).toInt(&okWidth);
            const int height = parts.value(1).toInt(&okHeight);
            if (!okWidth || !okHeight || width < 840 || height < 460) {
                std::fprintf(stderr, "invalid window size\n");
                return 2;
            }
            window.resize(width, height);
        }
        if (wantsScreenshot && !parser.isSet(windowSizeOption)) {
            // Keep the review canvas stable even when the offscreen platform
            // reports a small virtual screen.
            window.resize(1120, 720);
        }

        if (!wantsScreenshot) {
            window.show();
            return app.exec();
        }

        window.show();
        window.raise();
        const QString path = parser.isSet(nativeScreenshotOption)
            ? parser.value(nativeScreenshotOption)
            : parser.value(screenshotOption);
        const bool native = parser.isSet(nativeScreenshotOption);
        bool saved = false;
        QTimer::singleShot(300, &window, [&window, &saved, native, path]() {
            QPixmap pixmap;
            if (native) {
                if (QScreen* screen = window.screen()) {
                    pixmap = screen->grabWindow(window.winId());
                }
                if (pixmap.isNull()) {
                    std::fprintf(stderr, "native grab failed; falling back to widget grab\n");
                    pixmap = window.grab();
                }
            } else {
                pixmap = window.grab();
            }
            saved = savePixmap(pixmap, path);
            QCoreApplication::exit(saved ? 0 : 1);
        });
        return app.exec();
    };

    if (demoMode) {
        const std::optional<hustmcl::app::DemoScenario> scenario =
            hustmcl::app::DemoController::scenarioFromName(parser.value(scenarioOption));
        if (!scenario.has_value()) {
            std::fprintf(stderr, "unknown demo scenario\n");
            return 2;
        }
        hustmcl::app::DemoController controller;
        controller.setScenario(scenario.value());
        QApplication::setQuitOnLastWindowClosed(true);
        return runWindow(&controller, hustmcl::app::MainWindow::Mode::Demo, scenario);
    }

    const QString dataRoot =
        resolvePath(parser.value(dataRootOption), QCoreApplication::applicationDirPath());
    const QString javaPath =
        resolvePath(parser.value(javaOption), QCoreApplication::applicationDirPath());
    QString gameVersion = parser.value(gameVersionOption);
#ifdef HUSTMCL_PUBLIC_RELEASE
    if (!QFileInfo::exists(QDir(dataRoot).filePath("versions/" + gameVersion + "/" + gameVersion + ".json"))) gameVersion.clear();
#endif
    const QString username = parser.value(usernameOption);

    // Only the real mode needs to survive the last visible window; the
    // QtGameProcess owner keeps the event loop and game process alive.
    QApplication::setQuitOnLastWindowClosed(false);

    hustmcl::services::LocalGameInputSource source(dataRoot, gameVersion, javaPath);
    hustmcl::services::QtJavaProbe probe;
    hustmcl::application::LaunchPreparation preparation(source, probe);
    hustmcl::services::QtGameProcess process(dataRoot);
    hustmcl::services::JsonSettingsStore settingsStore(QDir(dataRoot).filePath(QStringLiteral("settings.json")));
    hustmcl::services::QtAccountService accountService;
    hustmcl::services::DpapiCredentialStore credentialStore(QDir(dataRoot).filePath(QStringLiteral("credentials/microsoft.dpapi")));
    hustmcl::application::AccountController account(accountService, credentialStore);
    hustmcl::application::OfflineController controller(
        preparation, process, dataRoot, gameVersion);
    auto settings = settingsStore.load().value_or(hustmcl::application::LauncherSettings{});
    if (parser.isSet(offlineOption)) settings.offlineMode = true;
    controller.applySettings(settings);
    // An explicit --username wins over the stored name for this run; it is only saved if the user later edits settings.
    if (parser.isSet(usernameOption) && !controller.setPlayerName(username)) {
        std::fprintf(stderr, "invalid offline player name\n");
        return 2;
    }
    controller.setSettingsStore(&settingsStore);
    QObject::connect(&accountService, &hustmcl::services::QtAccountService::authorizationRequired, &accountService,
        [&accountService](const QUrl& url) { if (!QDesktopServices::openUrl(url)) accountService.browserFailed(); });
    controller.setAccountController(&account);
    if (!wantsScreenshot) QTimer::singleShot(0, &controller, [&] {
        if (!controller.homeSnapshot().offlineMode) account.restore();
    });
    hustmcl::services::ModrinthContentSource contentSource({});
    hustmcl::services::QtFileAcquirer contentAcquirer({30000, false});
    hustmcl::services::QtContentDownloadStore contentFiles(dataRoot, contentAcquirer);
    hustmcl::application::ContentController content(contentSource, contentFiles);
    hustmcl::services::QtModpackWorkspace workspace(dataRoot, javaPath, {});
    hustmcl::application::ModpackController modpacks(workspace,
        {gameVersion, QStringLiteral("我的原版世界"), gameVersion, dataRoot, gameVersion, javaPath});
    if (!wantsScreenshot) QTimer::singleShot(0, &modpacks, &hustmcl::application::ModpackController::loadVanillaVersions);
    QObject::connect(&controller, &hustmcl::application::FrontendGateway::homeChanged, &modpacks,
        [&modpacks](const auto& home) { modpacks.setLaunchActive(home.sessionActive); });
    QObject::connect(&modpacks, &hustmcl::application::ModpackController::changed, &controller,
        [&controller](const auto& state) { controller.setImportActive(hustmcl::application::importActive(state.phase)); });
    QObject::connect(&modpacks, &hustmcl::application::ModpackController::instanceSelected, &controller,
        [&controller, &source, &process](const auto& instance) {
            if (!process.selectRoot(instance.root)) return;
            source.selectInstance(instance);
            controller.selectInstance(instance);
        });
    return runWindow(&controller, hustmcl::app::MainWindow::Mode::Real, std::nullopt, &content, &modpacks);
}
