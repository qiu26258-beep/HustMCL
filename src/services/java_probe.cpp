#include <hustmcl/services/java_probe.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace hustmcl::services {
using application::JavaProbeError;
using application::JavaProbeResult;

struct QtJavaProbe::Job : QObject {
    QString id;
    std::filesystem::path executable;
    Completion complete;
    QProcess* process = new QProcess(this);
    QTimer* timer = new QTimer(this);
    QByteArray output;
    std::optional<JavaProbeError> stopped;
    bool finished = false;
};

namespace {
bool hasExecutableHeader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { return false; }
    const auto dos = file.read(64);
    if (dos.size() != 64 || !dos.startsWith("MZ")) { return false; }
    quint32 offset = 0;
    for (int i = 0; i < 4; ++i) { offset |= static_cast<quint32>(static_cast<unsigned char>(dos[60 + i])) << (8 * i); }
    // Bounded support for normal Windows Java executables; no arbitrary file scan.
    if (offset < 64 || offset > 1024 * 1024 || file.size() < static_cast<qint64>(offset) + 24 || !file.seek(offset)) { return false; }
    return file.read(4) == QByteArray("PE\0\0", 4);
}

JavaProbeResult parseOutput(const QByteArray& bytes, const std::filesystem::path& executable)
{
    const auto lines = QString::fromUtf8(bytes).split('\n');
    QString majorText;
    QString architecture;
    for (const auto& raw : lines) {
        const auto line = raw.trimmed();
        if (line.startsWith(QLatin1String("java.specification.version"))) {
            if (!majorText.isEmpty()) { return JavaProbeError::InvalidOutput; }
            static const QRegularExpression property(QStringLiteral("^java\\.specification\\.version\\s*=\\s*([0-9]+(?:\\.[0-9]+)?)$"));
            const auto match = property.match(line);
            if (!match.hasMatch()) { return JavaProbeError::InvalidOutput; }
            majorText = match.captured(1);
        } else if (line.startsWith(QLatin1String("os.arch"))) {
            if (!architecture.isEmpty()) { return JavaProbeError::InvalidOutput; }
            static const QRegularExpression property(QStringLiteral("^os\\.arch\\s*=\\s*([A-Za-z0-9_]+)$"));
            const auto match = property.match(line);
            if (!match.hasMatch()) { return JavaProbeError::InvalidOutput; }
            architecture = match.captured(1);
        }
    }
    if (majorText.startsWith(QLatin1String("1."))) { majorText.remove(0, 2); }
    bool ok = false;
    const int major = majorText.toInt(&ok);
    if (!ok || major <= 0 || major > 1000 || architecture.isEmpty()) { return JavaProbeError::InvalidOutput; }
    core::RuntimeArchitecture arch = core::RuntimeArchitecture::Unknown;
    if (architecture == QLatin1String("amd64") || architecture == QLatin1String("x86_64")) {
        arch = core::RuntimeArchitecture::X64;
    } else if (architecture == QLatin1String("x86") || architecture == QLatin1String("i386")) {
        arch = core::RuntimeArchitecture::X86;
    } else if (architecture == QLatin1String("aarch64") || architecture == QLatin1String("arm64")) {
        arch = core::RuntimeArchitecture::Arm64;
    }
    return core::JavaRuntime{executable, major, arch};
}
} // namespace

QtJavaProbe::QtJavaProbe(QObject* parent, int timeoutMs, int maximumOutputBytes)
    : QObject(parent), timeoutMs_(std::clamp(timeoutMs, 1, 60000)),
      maximumOutputBytes_(std::clamp(maximumOutputBytes, 128, 1024 * 1024))
{
}

QtJavaProbe::~QtJavaProbe()
{
    for (auto* job : std::as_const(jobs_)) {
        job->complete = {};
        job->process->disconnect(this);
        job->timer->stop();
        job->process->kill();
        static_cast<void>(job->process->waitForFinished(1000));
    }
}

void QtJavaProbe::probe(const QString& operationId, const std::filesystem::path& executable, Completion complete)
{
    const auto reject = [this, complete](JavaProbeError error) {
        QTimer::singleShot(0, this, [complete, error] { complete(error); });
    };
    if (operationId.isEmpty() || !jobs_.isEmpty()) { reject(JavaProbeError::Busy); return; }
    const QString path = QString::fromStdWString(executable.wstring());
    const QFileInfo info(path);
    if (!QDir::isAbsolutePath(path) || path.startsWith(QLatin1String("\\\\")) || path.startsWith(QLatin1String("//"))
        || !path.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive) || !info.isFile()) {
        reject(JavaProbeError::InvalidPath); return;
    }
    if (!hasExecutableHeader(path)) { reject(JavaProbeError::FailedToStart); return; }
    auto* job = new Job;
    job->setParent(this);
    job->id = operationId;
    job->executable = executable;
    job->complete = std::move(complete);
    jobs_.insert(operationId, job);
    job->process->setProcessChannelMode(QProcess::MergedChannels);
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) {
        environment.remove(QLatin1String(key));
    }
    job->process->setProcessEnvironment(environment);
    job->process->setProgram(path);
    job->process->setArguments({QStringLiteral("-XshowSettings:properties"), QStringLiteral("-version")});
    job->process->setWorkingDirectory(info.absolutePath());
    connect(job->process, &QProcess::readyReadStandardOutput, this, [this, job] { consume(job); });
    connect(job->process, &QProcess::errorOccurred, this, [this, job](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) { finish(job, JavaProbeError::FailedToStart); }
    });
    connect(job->process, &QProcess::finished, this, [this, job](int exitCode, QProcess::ExitStatus status) {
        const QPointer<Job> guard(job);
        consume(job);
        if (!guard || guard->finished) { return; }
        if (job->stopped) { finish(job, *job->stopped); }
        else if (status != QProcess::NormalExit || exitCode != 0) { finish(job, JavaProbeError::AbnormalExit); }
        else { finish(job, parseOutput(job->output, job->executable)); }
    });
    job->timer->setSingleShot(true);
    connect(job->timer, &QTimer::timeout, this, [this, job] { stop(job, JavaProbeError::TimedOut); });
    job->timer->start(timeoutMs_);
    job->process->start();
}

void QtJavaProbe::consume(Job* job)
{
    if (job->finished) { return; }
    const auto bytes = job->process->readAllStandardOutput();
    if (bytes.size() > maximumOutputBytes_ - job->output.size()) {
        stop(job, JavaProbeError::OutputLimit);
        return;
    }
    job->output += bytes;
}

void QtJavaProbe::stop(Job* job, JavaProbeError reason)
{
    if (job->finished) { return; }
    if (!job->stopped) { job->stopped = reason; }
    if (job->process->state() == QProcess::NotRunning) { finish(job, *job->stopped); }
    else { job->process->kill(); }
}

void QtJavaProbe::finish(Job* job, JavaProbeResult result)
{
    if (job->finished) { return; }
    job->finished = true;
    job->timer->stop();
    job->process->disconnect(this);
    jobs_.remove(job->id);
    auto complete = std::move(job->complete);
    job->deleteLater();
    if (complete) { complete(std::move(result)); }
}

void QtJavaProbe::cancel(const QString& operationId)
{
    if (auto* job = jobs_.value(operationId, nullptr)) { stop(job, JavaProbeError::Cancelled); }
}

} // namespace hustmcl::services
