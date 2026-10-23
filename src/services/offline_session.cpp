#include <hustmcl/services/offline_session.h>
#include <hustmcl/services/local_game.h>
#include <hustmcl/services/modpack_workspace.h>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QUuid>

namespace hustmcl::services {
using namespace application;
LocalGameInputSource::LocalGameInputSource(QString root, QString version, QString java, QObject* parent)
    : QObject(parent), root_(std::move(root)), version_(std::move(version)), java_(std::move(java)), instanceId_(version_)
{ workers_.setMaxThreadCount(1); }
LocalGameInputSource::~LocalGameInputSource() { workers_.waitForDone(); }
void LocalGameInputSource::selectInstance(const application::InstalledInstance& instance)
{
    root_ = instance.root; version_ = instance.versionId; java_ = instance.javaPath; instanceId_ = instance.id;
    imported_ = instance.id.startsWith(QLatin1String("pack-"));
}
void LocalGameInputSource::load(const QString& instanceId, Completion complete)
{
    const QPointer<LocalGameInputSource> guard(this);
    const auto root = root_, version = version_, java = java_;
    const auto selected = instanceId_;
    const bool imported = imported_;
    workers_.start([guard, root, version, java, selected, imported, instanceId, complete = std::move(complete)]() mutable {
        auto read = [&]() -> LaunchInputResult {
            if (instanceId != selected) return PreparationError{PreparationErrorCode::InputUnavailable, {}, {}};
            if (imported && !verifyImportedPackFiles(root, true)) return PreparationError{PreparationErrorCode::InvalidInput, {}, {}};
            auto checked = verifyLocalGame(root, version);
            if (!std::holds_alternative<LocalGame>(checked)) return PreparationError{PreparationErrorCode::InvalidInput, {}, {}};
            LaunchInput result;
            result.version = std::get<LocalGame>(std::move(checked)).version;
            result.context.dataRoot = root.toStdWString();
            result.context.gameDirectory = result.context.dataRoot / "game";
            result.context.nativesDirectory = result.context.dataRoot / "natives";
            result.manualJava = java.toStdWString();
            return result;
        };
        auto result = read();
        if (guard) QMetaObject::invokeMethod(guard.data(), [guard, complete = std::move(complete), result = std::move(result)]() mutable {
            if (guard) complete(std::move(result));
        }, Qt::QueuedConnection);
    });
}
QtGameProcess::QtGameProcess(QString root, QObject* parent) : QObject(parent), root_(std::move(root))
{
    startTimeout_.setSingleShot(true);
    connect(&startTimeout_, &QTimer::timeout, this, [this] {
        // The application retains this owner while active, including after window close.
        if (game_.state() != QProcess::NotRunning) game_.kill();
        else fail(GameProcessError::FailedToStart);
    });
    connect(&game_, &QProcess::started, this, [this] {
        startTimeout_.stop();
        const auto callback = observer_;
        if (callback) callback({GameProcessEvent::Kind::Started, logBase_});
    });
    connect(&game_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) fail(GameProcessError::FailedToStart);
    });
    connect(&game_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        finish(status == QProcess::NormalExit, code);
    });
}
bool QtGameProcess::selectRoot(const QString& root)
{
    if (observer_ || game_.state() != QProcess::NotRunning) return false;
    root_ = root;
    return true;
}
void QtGameProcess::start(const QString&, const core::LaunchPlan& plan, const OfflineLaunch& identity, Observer observer)
{
    if (observer_ || game_.state() != QProcess::NotRunning) {
        observer({GameProcessEvent::Kind::Failed, {}, 0, GameProcessError::Busy});
        return;
    }
    observer_ = std::move(observer);
    identity_ = identity;
    logBase_.clear();
    const QDir data(root_);
    if (QDir::cleanPath(QString::fromStdWString(plan.workingDirectory.wstring())).compare(
            QDir::cleanPath(data.filePath("game")), Qt::CaseInsensitive) != 0) {
        fail(GameProcessError::UnsafeDirectory); return;
    }
    lock_ = std::make_unique<QLockFile>(data.filePath(".offline.lock"));
    lock_->setStaleLockTime(0);
    if (!lock_->tryLock(0)) { fail(GameProcessError::Busy); return; }
    if (!prepareLocalGameDirectories(root_)) { fail(GameProcessError::UnsafeDirectory); return; }
    logBase_ = data.filePath("launcher-logs/" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) environment.remove(QLatin1String(key));
    game_.setProcessEnvironment(environment);
    game_.setWorkingDirectory(data.filePath("game"));
    game_.setStandardOutputFile(logBase_ + ".stdout.log");
    game_.setStandardErrorFile(logBase_ + ".stderr.log");
    startTimeout_.start(10000);
    game_.start(QString::fromStdWString(plan.runtime.executable.wstring()), identity.arguments);
}
void QtGameProcess::finish(bool normal, int exitCode)
{
    if (!observer_) return;
    startTimeout_.stop();
    QSaveFile report(logBase_ + ".json");
    const auto bytes = QJsonDocument(QJsonObject{{"mode", "offline-gui"}, {"authenticated", false}, {"demo", false},
        {"playerName", identity_.playerName}, {"offlineUuid", identity_.uuid},
        {"exitCode", exitCode}, {"normalExit", normal}}).toJson();
    const bool saved = report.open(QIODevice::WriteOnly) && report.write(bytes) == bytes.size() && report.commit();
    auto callback = std::move(observer_);
    observer_ = {};
    lock_.reset();
    callback({normal && exitCode == 0 && saved ? GameProcessEvent::Kind::Finished : GameProcessEvent::Kind::Failed,
              logBase_, exitCode, saved ? GameProcessError::AbnormalExit : GameProcessError::ReportFailed});
}
void QtGameProcess::fail(GameProcessError error)
{
    if (!observer_) return;
    startTimeout_.stop();
    auto callback = std::move(observer_);
    observer_ = {};
    lock_.reset();
    callback({GameProcessEvent::Kind::Failed, logBase_, 0, error});
}
}
