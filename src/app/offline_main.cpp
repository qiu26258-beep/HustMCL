#include <hustmcl/application/offline_launch.h>
#include <hustmcl/services/local_game.h>
#include <hustmcl/services/java_probe.h>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QUuid>
#include <iostream>

using namespace hustmcl;
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("HustMCL Offline");
    QCommandLineParser parser;
    parser.setApplicationDescription("Local offline single-player launcher. No authenticated account.");
    parser.addHelpOption();
    const QDir base(QCoreApplication::applicationDirPath());
    parser.addOptions({
        {"data-root", "Existing verified game data directory.", "path", base.filePath("data")},
        {"java", "Java executable.", "path", base.filePath("runtime/bin/java.exe")},
        {"username", "Offline player name: 1-16 ASCII letters, digits or underscores.", "name", "HustPlayer"},
        {"version", "Installed version identifier.", "id", "26.2"},
        {"check-only", "Verify files, Java and launch arguments without starting Minecraft."}
    });
    parser.process(app);
    const auto name = parser.value("username");
    if (std::holds_alternative<application::OfflineLaunchError>(application::makeOfflineLaunch({}, name))) {
        std::cerr << "Invalid offline player name.\n"; return 2;
    }
    const auto root = QDir::cleanPath(parser.value("data-root"));
    std::cout << "Checking local game files..." << std::endl;
    auto checked = services::verifyLocalGame(root, parser.value("version"));
    if (const auto error = std::get_if<services::LocalGameError>(&checked)) {
        std::cerr << "Local game check failed (" << static_cast<int>(*error) << "). No game started.\n"; return 3;
    }
    const auto& installed = std::get<services::LocalGame>(checked);
    services::QtJavaProbe java;
    QEventLoop loop;
    bool completed = false;
    application::JavaProbeResult runtime = application::JavaProbeError::InvalidOutput;
    java.probe("offline-java", parser.value("java").toStdWString(), [&](auto result) {
        runtime = std::move(result); completed = true; loop.quit();
    });
    if (!completed) loop.exec();
    if (!std::holds_alternative<core::JavaRuntime>(runtime)) { std::cerr << "Java probe failed.\n"; return 4; }
    core::LaunchContext context;
    context.dataRoot = root.toStdWString();
    context.gameDirectory = context.dataRoot / "game";
    context.nativesDirectory = context.dataRoot / "natives";
    context.runtimes = {std::get<core::JavaRuntime>(runtime)};
    auto plan = core::buildLaunchPlan(installed.version, context);
    if (!std::holds_alternative<core::LaunchPlan>(plan)) { std::cerr << "Launch plan rejected.\n"; return 5; }
    const auto& launchPlan = std::get<core::LaunchPlan>(plan);
    auto resolved = application::makeOfflineLaunch(launchPlan, name);
    if (!std::holds_alternative<application::OfflineLaunch>(resolved)) return 5;
    const auto& launch = std::get<application::OfflineLaunch>(resolved);
    std::cout << "Offline player: " << launch.playerName.toStdString() << " (" << launch.uuid.toStdString() << ")\n"
        << "Verified files: " << installed.files.files.size() << "\n" << std::flush;
    if (parser.isSet("check-only")) return 0;
    const QDir data(root);
    QLockFile lock(data.filePath(".offline.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) { std::cerr << "This installation is already in use.\n"; return 6; }
    if (!services::prepareLocalGameDirectories(root)) { std::cerr << "Unsafe game directory.\n"; return 7; }
    const auto runId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto logBase = data.filePath("launcher-logs/" + runId);
    QProcess game;
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) environment.remove(QLatin1String(key));
    game.setProcessEnvironment(environment);
    game.setWorkingDirectory(data.filePath("game"));
    game.setStandardOutputFile(logBase + ".stdout.log");
    game.setStandardErrorFile(logBase + ".stderr.log");
    game.start(QString::fromStdWString(launchPlan.runtime.executable.wstring()), launch.arguments);
    if (!game.waitForStarted(10000)) { std::cerr << "Game process failed to start.\n"; return 8; }
    std::cout << "Minecraft running. Saves: " << data.filePath("game/saves").toStdString() << std::endl;
    while (!game.waitForFinished(1000)) {
        if (game.state() == QProcess::NotRunning) break;
    }
    QSaveFile report(logBase + ".json");
    const auto bytes = QJsonDocument(QJsonObject{{"mode", "offline"}, {"authenticated", false}, {"demo", false},
        {"playerName", name}, {"offlineUuid", launch.uuid}, {"verifiedFiles", static_cast<int>(installed.files.files.size())},
        {"exitCode", game.exitCode()}, {"normalExit", game.exitStatus() == QProcess::NormalExit}}).toJson();
    if (!report.open(QIODevice::WriteOnly) || report.write(bytes) != bytes.size() || !report.commit()) return 9;
    std::cout << "Game exited: " << game.exitCode() << ". Report: " << (logBase + ".json").toStdString() << std::endl;
    return game.exitStatus() == QProcess::NormalExit && game.exitCode() == 0 ? 0 : 10;
}
