// Developer-only local-copy experiment; not a product installation transaction.
#include <hustmcl/services/install_plan_input.h>
#include <hustmcl/services/version_metadata.h>
#include <hustmcl/services/java_probe.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QTemporaryDir>
#include <iostream>
using namespace hustmcl;
static QByteArray read(const QString& path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
static bool verify(const QString& path, const core::DownloadDescriptor& d) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly) || f.size() != static_cast<qint64>(d.size)) return false;
    QCryptographicHash h(QCryptographicHash::Sha1);
    return h.addData(&f) && h.result().toHex().toStdString() == d.sha1;
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 4) { std::cerr << "usage: local_demo_probe SOURCE_ROOT JAVA_EXE OUTPUT_PARENT\n"; return 2; }
    const QDir source(args[1]);
    const auto metadataPath = source.filePath("versions/26.2/26.2.json");
    const auto metadata = read(metadataPath);
    auto parsed = services::parseVersionMetadata(metadata);
    if (!std::holds_alternative<core::VersionMetadata>(parsed)) return 3;
    const auto& version = std::get<core::VersionMetadata>(parsed);
    const auto hash = QCryptographicHash::hash(metadata, QCryptographicHash::Sha1).toHex();
    auto loaded = services::loadInstallFilePlan(metadataPath, hash, source.filePath("assets/indexes/32.json"));
    if (!std::holds_alternative<core::InstallFilePlan>(loaded)) return 3;
    services::QtJavaProbe probe;
    QEventLoop probeLoop;
    application::JavaProbeResult runtime = application::JavaProbeError::InvalidOutput;
    bool completed = false;
    probe.probe("local-demo-java", args[2].toStdWString(), [&](auto result) {
        runtime = std::move(result); completed = true; probeLoop.quit();
    });
    if (!completed) probeLoop.exec();
    if (!std::holds_alternative<core::JavaRuntime>(runtime)) return 4;
    QTemporaryDir target(QDir(args[3]).filePath("demo-26.2-XXXXXX"));
    if (!target.isValid()) return 5;
    target.setAutoRemove(false); // Explicit evidence retained; never touch the source game directory.
    const QDir destination(target.path());
    std::cout << "Test directory: " << target.path().toStdString() << std::endl;
    const auto& plan = std::get<core::InstallFilePlan>(loaded);
    for (const auto& file : plan.files) {
        const auto relative = QString::fromStdWString(file.relativePath.wstring());
        const auto from = source.filePath(relative), to = destination.filePath(relative);
        if (!verify(from, file.download) || !QDir().mkpath(QFileInfo(to).absolutePath()) || !QFile::copy(from, to) || !verify(to, file.download)) {
            std::cerr << "copy/verify failed: " << relative.toStdString() << '\n'; return 6;
        }
    }
    if (!QFile::copy(metadataPath, destination.filePath("versions/26.2/26.2.json"))) return 6;
    core::LaunchContext context;
    context.dataRoot = target.path().toStdWString();
    context.gameDirectory = context.dataRoot / "game";
    context.nativesDirectory = context.dataRoot / "natives";
    context.runtimes = {std::get<core::JavaRuntime>(runtime)};
    auto launch = core::buildLaunchPlan(version, context);
    if (!std::holds_alternative<core::LaunchPlan>(launch)) return 7;
    QStringList command;
    for (const auto& argument : std::get<core::LaunchPlan>(launch).arguments) {
        QString value;
        for (const auto& part : argument) {
            if (const auto literal = std::get_if<std::string>(&part)) value += QString::fromStdString(*literal);
            else switch (std::get<core::SessionField>(part)) {
                case core::SessionField::PlayerName: value += "HustMCLDemo"; break;
                case core::SessionField::Uuid: value += "00000000000000000000000000000000"; break;
                default: value += "0"; break;
            }
        }
        command.append(value);
    }
    command.append("--demo"); // No authenticated session; never expose a normal-play entry.
    if (!QDir().mkpath(destination.filePath("game")) || !QDir().mkpath(destination.filePath("natives"))) return 6;
    QProcess game;
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) {
        environment.remove(QLatin1String(key));
    }
    game.setProcessEnvironment(environment);
    game.setWorkingDirectory(destination.filePath("game"));
    game.setStandardOutputFile(destination.filePath("stdout.log"));
    game.setStandardErrorFile(destination.filePath("stderr.log"));
    game.start(QString::fromStdWString(std::get<core::LaunchPlan>(launch).runtime.executable.wstring()), command);
    if (!game.waitForStarted(10000)) return 8;
    std::cout << "Verified " << plan.files.size() << " files. Demo directory: " << target.path().toStdString() << std::endl;
    // User can inspect the actual window; close normally after the bounded experiment.
    const bool exited = game.waitForFinished(90000);
    if (!exited) { game.terminate(); if (!game.waitForFinished(10000)) { game.kill(); game.waitForFinished(); } }
    QSaveFile report(destination.filePath("result.json"));
    if (!report.open(QIODevice::WriteOnly)) return 9;
    const auto bytes = QJsonDocument(QJsonObject{{"verifiedFiles", static_cast<int>(plan.files.size())},
        {"verifiedBytes", QString::number(plan.totalBytes)}, {"metadataSha1", QString::fromLatin1(hash)},
        {"demo", true}, {"productInstallationTransaction", false}, {"exitedBeforeTimeout", exited},
        {"exitCode", game.exitCode()}, {"normalExit", game.exitStatus() == QProcess::NormalExit},
        {"mainMenuVisuallyVerified", false}}).toJson();
    if (report.write(bytes) != bytes.size() || !report.commit()) return 9;
    if (!exited) return 11;
    return game.exitStatus() == QProcess::NormalExit && game.exitCode() == 0 ? 0 : 10;
}
