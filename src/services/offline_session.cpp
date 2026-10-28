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
    connect(&game_, &QProcess::readyReadStandardOutput, this, [this] { captureOutput(false); });
    connect(&game_, &QProcess::readyReadStandardError, this, [this] { captureOutput(true); });
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
    stdoutBuffer_.clear(); stderrBuffer_.clear(); logFailed_ = false;
    stdoutLog_.setFileName(logBase_ + ".stdout.log"); stderrLog_.setFileName(logBase_ + ".stderr.log");
    if (!stdoutLog_.open(QIODevice::WriteOnly | QIODevice::NewOnly) || !stderrLog_.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        fail(GameProcessError::ReportFailed); return;
    }
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) environment.remove(QLatin1String(key));
    game_.setProcessEnvironment(environment);
    game_.setWorkingDirectory(data.filePath("game"));
    startTimeout_.start(10000);
    game_.start(QString::fromStdWString(plan.runtime.executable.wstring()), identity.arguments);
}
void QtGameProcess::captureOutput(bool standardError, bool final)
{
    auto& buffer = standardError ? stderrBuffer_ : stdoutBuffer_;
    auto& file = standardError ? stderrLog_ : stdoutLog_;
    buffer.append(standardError ? game_.readAllStandardError() : game_.readAllStandardOutput());
    qsizetype overlap = 0;
    for (const auto& secret : identity_.redactions) overlap = qMax(overlap, secret.toUtf8().size() - 1);
    const auto limit = final ? buffer.size() : qMax(qsizetype(0), buffer.size() - overlap);
    QByteArray clean;
    qsizetype cursor = 0;
    while (cursor < limit) {
        qsizetype found = -1, length = 0;
        for (const auto& secret : identity_.redactions) {
            const auto bytes = secret.toUtf8();
            if (bytes.isEmpty()) continue;
            const auto at = buffer.indexOf(bytes, cursor);
            if (at >= 0 && at < limit && (found < 0 || at < found || (at == found && bytes.size() > length))) { found = at; length = bytes.size(); }
        }
        if (found < 0) { clean.append(buffer.mid(cursor, limit - cursor)); cursor = limit; }
        else { clean.append(buffer.mid(cursor, found - cursor)); clean.append("[REDACTED]"); cursor = found + length; }
    }
    buffer.remove(0, cursor);
    if (!clean.isEmpty() && (!file.isOpen() || file.write(clean) != clean.size())) logFailed_ = true;
}
void QtGameProcess::finish(bool normal, int exitCode)
{
    if (!observer_) return;
    startTimeout_.stop();
    captureOutput(false, true); captureOutput(true, true);
    if (!stdoutLog_.flush() || !stderrLog_.flush()) logFailed_ = true;
    stdoutLog_.close(); stderrLog_.close();
    QSaveFile report(logBase_ + ".json");
    QJsonObject summary{{"mode", identity_.authenticated ? "authenticated-gui" : "offline-gui"}, {"authenticated", identity_.authenticated}, {"demo", false},
        {"playerName", identity_.playerName},
        {"exitCode", exitCode}, {"normalExit", normal}};
    if (!identity_.authenticated) summary.insert("offlineUuid", identity_.uuid);
    const auto bytes = QJsonDocument(summary).toJson();
    const bool saved = !logFailed_ && report.open(QIODevice::WriteOnly) && report.write(bytes) == bytes.size() && report.commit();
    auto callback = std::move(observer_);
    observer_ = {};
    identity_ = {}; game_.setArguments({});
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
    stdoutLog_.close(); stderrLog_.close();
    stdoutBuffer_.clear(); stderrBuffer_.clear(); identity_ = {}; game_.setArguments({});
    lock_.reset();
    callback({GameProcessEvent::Kind::Failed, logBase_, 0, error});
}
}
