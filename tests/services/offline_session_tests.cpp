#include <hustmcl/services/offline_session.h>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
using namespace hustmcl;
using namespace application;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
QByteArray read(const QString& path) { QFile file(path); require(file.open(QIODevice::ReadOnly), "open output"); return file.readAll(); }
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        require(argc == 2, "fixture path argument");
        QTemporaryDir temporary;
        require(temporary.isValid(), "temporary root");
        const auto root = temporary.path();
        services::QtGameProcess process(root), second(root);
        core::LaunchPlan plan;
        plan.runtime.executable = QString::fromLocal8Bit(argv[1]).toStdWString();
        plan.workingDirectory = QDir(root).filePath("game").toStdWString();
        OfflineLaunch identity{"Test_Player", "test-uuid", {"literal ; $(echo unsafe)", "带空格 path"}};
        qputenv("JAVA_TOOL_OPTIONS", "injected"); qputenv("CLASSPATH", "injected");
        for (bool fail : {false, true}) {
            if (fail) identity.arguments.append("--fail");
            QEventLoop loop;
            QTimer timeout; timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            bool started = false, done = false, blocked = false;
            GameProcessEvent result{GameProcessEvent::Kind::Failed, {}};
            process.start("task", plan, identity, [&](GameProcessEvent event) {
                if (event.kind == GameProcessEvent::Kind::Started) {
                    started = true;
                    require(!process.selectRoot("D:/unrelated"), "running process root cannot change");
                    second.start("other", plan, identity, [&](auto busy) { blocked = busy.error == GameProcessError::Busy && busy.kind == GameProcessEvent::Kind::Failed; });
                } else { result = std::move(event); done = true; loop.quit(); }
            });
            timeout.start(5000);
            if (!done) loop.exec();
            require(started && done && blocked, "real process starts, instance lock excludes second owner, eventually exits");
            require(result.kind == (fail ? GameProcessEvent::Kind::Failed : GameProcessEvent::Kind::Finished), "nonzero exit is failure");
            const auto report = QJsonDocument::fromJson(read(result.logPath + ".json")).object();
            require(report["exitCode"].toInt() == (fail ? 7 : 0) && !report["authenticated"].toBool() && !report["demo"].toBool(), "exit report is accurate");
            const auto output = QJsonDocument::fromJson(read(result.logPath + ".stdout.log")).object();
            require(output["javaOptions"].toString().isEmpty() && output["classpath"].toString().isEmpty(), "Java injection environment removed");
            require(output["arguments"].toArray().first().toString() == identity.arguments.first(), "argument boundary preserved without shell");
        }
        {
            auto online = identity;
            online.arguments = {"--echo-secret", "private-session-token"};
            online.authenticated = true; online.redactions = {"private-session-token"};
            QEventLoop loop;
            QString log;
            process.start("online", plan, online, [&](auto event) {
                if (event.kind == GameProcessEvent::Kind::Finished) { log = event.logPath; loop.quit(); }
                if (event.kind == GameProcessEvent::Kind::Failed) loop.quit();
            });
            QTimer::singleShot(5000, &loop, &QEventLoop::quit); loop.exec();
            require(!log.isEmpty(), "authenticated process exits normally");
            const auto report = QJsonDocument::fromJson(read(log + ".json")).object();
            require(report["authenticated"].toBool() && !report.contains("offlineUuid"), "online report accurately distinguishes authenticated identity");
            for (const auto& extension : {".stdout.log", ".stderr.log"}) {
                const auto output = read(log + extension);
                require(!output.contains("private-session-token") && output.contains("[REDACTED]"), "secrets split over output chunks are redacted");
            }
        }
        bool rejected = false;
        require(process.selectRoot(root), "idle process may select a verified instance root");
        plan.workingDirectory = L"C:\\Windows";
        process.start("unsafe", plan, identity, [&](auto event) { rejected = event.error == GameProcessError::UnsafeDirectory; });
        require(rejected, "unrelated working directory rejected");
        std::cout << "offline session process scenarios passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
