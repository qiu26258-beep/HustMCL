#pragma once

#include <hustmcl/core/launch_plan.h>

#include <QObject>
#include <QString>
#include <QMetaType>

#include <functional>
#include <optional>

namespace hustmcl::application {

enum class JavaProbeError { InvalidPath, FailedToStart, TimedOut, OutputLimit, AbnormalExit, InvalidOutput, Cancelled, Busy };
using JavaProbeResult = std::variant<core::JavaRuntime, JavaProbeError>;

class JavaRuntimeProbe {
public:
    using Completion = std::function<void(JavaProbeResult)>;
    virtual ~JavaRuntimeProbe() = default;
    // Completion is delivered on the owning event thread; cancellation completes after process exit.
    virtual void probe(const QString& operationId, const std::filesystem::path& executable, Completion complete) = 0;
    virtual void cancel(const QString& operationId) = 0;
};

enum class PreparationErrorCode { InputUnavailable, InvalidInput, JavaUnavailable, InvalidManualJava, InvalidPlan };
struct PreparationError {
    PreparationErrorCode code;
    std::optional<JavaProbeError> probeError;
    std::optional<core::PlanError> planError;
};

struct LaunchInput {
    core::VersionMetadata version;
    core::LaunchContext context; // Runtime fields are discarded; only actual probe results are used.
    std::vector<std::filesystem::path> javaCandidates;
    std::optional<std::filesystem::path> manualJava;
};
using LaunchInputResult = std::variant<LaunchInput, PreparationError>;

class LaunchInputSource {
public:
    using Completion = std::function<void(LaunchInputResult)>;
    virtual ~LaunchInputSource() = default;
    // Read-only snapshot; no credential/config/cache writes. Late callbacks may be discarded.
    virtual void load(const QString& instanceId, Completion complete) = 0;
};

enum class PreparationPhase { Idle, LoadingInput, ProbingJava, Planning, Prepared, Cancelling, Cancelled, Failed };
struct PreparationSnapshot {
    QString taskId;
    QString instanceId;
    PreparationPhase phase = PreparationPhase::Idle;
    bool cancelAvailable = false;
    std::optional<PreparationError> error;
};

class LaunchPreparation final : public QObject {
    Q_OBJECT
public:
    LaunchPreparation(LaunchInputSource& source, JavaRuntimeProbe& probe, QObject* parent = nullptr);
    ~LaunchPreparation() override;
    [[nodiscard]] PreparationSnapshot snapshot() const { return state_; }
    [[nodiscard]] bool start(const QString& instanceId);
    [[nodiscard]] bool cancel(const QString& taskId);
    // Applied to plans built after this call; an in-flight task keeps the value it started with.
    void setMemoryMb(int memoryMb) { memoryMb_ = memoryMb; }

signals:
    void changed(const hustmcl::application::PreparationSnapshot& snapshot);
    void prepared(const QString& taskId, const hustmcl::core::LaunchPlan& plan);

private:
    bool isCurrent(const QString& taskId, PreparationPhase phase) const;
    bool transition(const QString& taskId, PreparationPhase phase);
    void inputArrived(const QString& taskId, LaunchInputResult input);
    void nextJava(const QString& taskId);
    void javaArrived(const QString& taskId, const QString& operationId, JavaProbeResult result);
    void makePlan(const QString& taskId);
    void fail(const QString& taskId, PreparationError error);

    LaunchInputSource& source_;
    JavaRuntimeProbe& probe_;
    PreparationSnapshot state_;
    std::optional<LaunchInput> input_;
    std::vector<std::filesystem::path> candidates_;
    std::size_t candidateIndex_ = 0;
    QString operationId_;
    std::optional<JavaProbeError> lastProbeError_;
    bool shuttingDown_ = false;
    int memoryMb_ = 0; // 0 keeps the source's own value
    int taskMemoryMb_ = 0;
};

} // namespace hustmcl::application

Q_DECLARE_METATYPE(hustmcl::application::PreparationSnapshot)
Q_DECLARE_METATYPE(hustmcl::core::LaunchPlan)
