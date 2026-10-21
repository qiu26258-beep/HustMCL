#include "demo_controller.h"
#include "main_window.h"

#include "theme.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QStyleFactory>
#include <QTimer>

#include <cstdio>

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
    QApplication::setApplicationVersion(QStringLiteral("0.1.0-demo"));
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setFont(hustmcl::ui::theme::baseFont());
    app.setStyleSheet(hustmcl::ui::theme::styleSheet());
    Q_INIT_RESOURCE(resources);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("HustMCL Campus Desk Qt Widgets demo"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption scenarioOption(
        QStringLiteral("scenario"),
        QStringLiteral("Demo state: home-ready, install-unknown, install-known, install-cancelling, "
                       "install-cancelled, install-committing, install-failed, install-long-path."),
        QStringLiteral("name"),
        QStringLiteral("home-ready"));
    QCommandLineOption pageOption(
        QStringLiteral("page"),
        QStringLiteral("Initial page: home or install."),
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
    parser.addOption(pageOption);
    parser.addOption(screenshotOption);
    parser.addOption(nativeScreenshotOption);
    parser.addOption(windowSizeOption);
    parser.addOption(scaleOption);
    parser.addOption(noDecorationOption);
    parser.process(app);

    const std::optional<hustmcl::app::DemoScenario> scenario =
        hustmcl::app::DemoController::scenarioFromName(parser.value(scenarioOption));
    if (!scenario.has_value()) {
        std::fprintf(stderr, "unknown demo scenario\n");
        return 2;
    }

    hustmcl::app::DemoController controller;
    controller.setScenario(scenario.value());

    hustmcl::app::MainWindow window(&controller);
    const bool noDecoration = parser.isSet(noDecorationOption);
    window.setDecorationsVisible(!noDecoration);

    const QString pageValue = parser.value(pageOption).toLower();
    hustmcl::app::MainWindow::Page page = hustmcl::app::MainWindow::Page::Home;
    if (pageValue == QStringLiteral("install")) {
        page = hustmcl::app::MainWindow::Page::Install;
    } else if (pageValue == QStringLiteral("home")) {
        page = hustmcl::app::MainWindow::Page::Home;
    } else if (parser.value(scenarioOption).startsWith(QStringLiteral("install"))) {
        page = hustmcl::app::MainWindow::Page::Install;
    }
    window.setCurrentPage(page);

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

    const bool wantsScreenshot = parser.isSet(screenshotOption) || parser.isSet(nativeScreenshotOption);
    if (parser.isSet(screenshotOption) && parser.isSet(nativeScreenshotOption)) {
        std::fprintf(stderr, "choose one screenshot mode\n");
        return 2;
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

    QTimer::singleShot(300, &window, [&window, &app, &saved, native, path]() {
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
}
