#include <hustmcl/application/launch_preparation.h>
#include <hustmcl/services/java_probe.h>
#include <hustmcl/services/launch_input_source.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimer>

#include <iostream>

using namespace hustmcl;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("hustmcl_prepare_probe");
    QCommandLineParser parser;
    parser.setApplicationDescription("Read-only developer launch preparation probe; never starts Minecraft.");
    parser.addHelpOption();
    parser.addOptions({
        {{"m", "metadata"}, "Absolute path to pinned modern release metadata.", "path"},
        {{"s", "sha1"}, "Expected SHA-1 from the trusted metadata source.", "digest"},
        {{"j", "java"}, "Absolute path to the Java executable to actually probe.", "path"},
        {{"o", "output"}, "Optional JSON report file (parent directory must exist).", "path"}
    });
    parser.process(app);
    for (const auto* option : {"metadata", "sha1", "java"}) {
        if (!parser.isSet(QLatin1String(option))) { parser.showHelp(2); }
    }
    application::LaunchInput settings;
    const auto root = std::filesystem::path(QDir::current().absoluteFilePath("out/prepare-probe-data").toStdWString());
    settings.context.dataRoot = root;
    settings.context.gameDirectory = root / "instance/game";
    settings.context.nativesDirectory = root / "instance/natives";
    settings.manualJava = std::filesystem::path(parser.value("java").toStdWString());
    services::MetadataFileSource source("probe-instance", parser.value("metadata"), parser.value("sha1").toLatin1(), settings);
    services::QtJavaProbe java;
    application::LaunchPreparation preparation(source, java);
    bool finished = false;
    const auto finish = [&](QJsonObject result, int exitCode) {
        if (finished) { return; }
        finished = true;
        result.insert("realGameStarted", false);
        result.insert("installationVerified", false);
        const auto bytes = QJsonDocument(result).toJson();
        if (parser.isSet("output")) {
            QSaveFile output(parser.value("output"));
            if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) { exitCode = 3; }
        }
        std::cout << bytes.constData();
        app.exit(exitCode);
    };
    QObject::connect(&preparation, &application::LaunchPreparation::prepared, &app,
                     [&](const QString&, const core::LaunchPlan& plan) {
        finish({{"status", "prepared"}, {"javaMajor", plan.runtime.majorVersion}, {"architecture", "x64"},
                {"classpathEntries", static_cast<int>(plan.classpath.size())},
                {"fileRequirements", static_cast<int>(plan.files.size())},
                {"argumentCount", static_cast<int>(plan.arguments.size())}}, 0);
    });
    QObject::connect(&preparation, &application::LaunchPreparation::changed, &app,
                     [&](const application::PreparationSnapshot& state) {
        if (state.phase == application::PreparationPhase::Failed) {
            QJsonObject error{{"status", "failed"}, {"errorCode", static_cast<int>(state.error->code)}};
            if (state.error->probeError) { error.insert("javaError", static_cast<int>(*state.error->probeError)); }
            if (state.error->planError) { error.insert("planError", static_cast<int>(state.error->planError->code)); }
            finish(error, 1);
        } else if (state.phase == application::PreparationPhase::Cancelled) { finish({{"status", "cancelled"}}, 1); }
    });
    QTimer::singleShot(0, &app, [&] { static_cast<void>(preparation.start("probe-instance")); });
    QTimer::singleShot(20000, &app, [&] { static_cast<void>(preparation.cancel(preparation.snapshot().taskId)); });
    return app.exec();
}
