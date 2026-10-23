#include <hustmcl/application/launch_preparation.h>

#include <QPointer>
#include <QUuid>

#include <utility>

namespace hustmcl::application {
namespace {
bool active(PreparationPhase phase)
{
    return phase == PreparationPhase::LoadingInput || phase == PreparationPhase::ProbingJava
        || phase == PreparationPhase::Planning || phase == PreparationPhase::Cancelling;
}
} // namespace

LaunchPreparation::LaunchPreparation(LaunchInputSource& source, JavaRuntimeProbe& probe, QObject* parent)
    : QObject(parent), source_(source), probe_(probe)
{
    qRegisterMetaType<PreparationSnapshot>();
    qRegisterMetaType<core::LaunchPlan>();
}

LaunchPreparation::~LaunchPreparation()
{
    shuttingDown_ = true;
    if (!operationId_.isEmpty()) { probe_.cancel(operationId_); }
}

bool LaunchPreparation::isCurrent(const QString& taskId, PreparationPhase phase) const
{
    return !shuttingDown_ && state_.taskId == taskId && state_.phase == phase;
}

bool LaunchPreparation::transition(const QString& taskId, PreparationPhase phase)
{
    state_.phase = phase;
    state_.cancelAvailable = phase == PreparationPhase::LoadingInput || phase == PreparationPhase::ProbingJava
        || phase == PreparationPhase::Planning;
    const QPointer<LaunchPreparation> guard(this);
    const auto snapshot = state_;
    emit changed(snapshot);
    // Direct signal receivers may cancel, restart, or destroy the use case.
    return guard && guard->isCurrent(taskId, phase);
}

bool LaunchPreparation::start(const QString& instanceId)
{
    if (active(state_.phase) || instanceId.trimmed().isEmpty()) { return false; }
    state_ = {};
    state_.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    state_.instanceId = instanceId;
    input_.reset();
    candidates_.clear();
    candidateIndex_ = 0;
    operationId_.clear();
    lastProbeError_.reset();
    const auto taskId = state_.taskId;
    if (!transition(taskId, PreparationPhase::LoadingInput)) { return true; }
    const QPointer<LaunchPreparation> guard(this);
    source_.load(instanceId, [guard, taskId](LaunchInputResult input) {
        if (guard) { guard->inputArrived(taskId, std::move(input)); }
    });
    return true;
}

bool LaunchPreparation::cancel(const QString& taskId)
{
    if (state_.taskId != taskId || !state_.cancelAvailable) { return false; }
    if (state_.phase == PreparationPhase::ProbingJava) {
        if (transition(taskId, PreparationPhase::Cancelling)) { probe_.cancel(operationId_); }
    } else {
        static_cast<void>(transition(taskId, PreparationPhase::Cancelled));
    }
    return true;
}

void LaunchPreparation::inputArrived(const QString& taskId, LaunchInputResult result)
{
    if (!isCurrent(taskId, PreparationPhase::LoadingInput)) { return; }
    if (const auto error = std::get_if<PreparationError>(&result)) { fail(taskId, *error); return; }
    input_ = std::get<LaunchInput>(std::move(result));
    input_->context.runtimes.clear();
    input_->context.manualRuntime.reset();
    const auto requirement = core::selectJavaRuntime(input_->version.java, {});
    if (std::get<core::JavaSelectionError>(requirement) == core::JavaSelectionError::InvalidRequirement) {
        fail(taskId, {PreparationErrorCode::InvalidInput, {}, core::PlanError{core::PlanErrorCode::InvalidMetadata, "javaVersion"}});
        return;
    }
    candidates_ = input_->manualJava ? std::vector{*input_->manualJava} : input_->javaCandidates;
    nextJava(taskId);
}

void LaunchPreparation::nextJava(const QString& taskId)
{
    if (candidateIndex_ >= candidates_.size()) {
        fail(taskId, {input_->manualJava ? PreparationErrorCode::InvalidManualJava : PreparationErrorCode::JavaUnavailable,
                      lastProbeError_, {}});
        return;
    }
    operationId_ = taskId + '/' + QString::number(static_cast<qulonglong>(candidateIndex_));
    const auto operationId = operationId_;
    const auto executable = candidates_[candidateIndex_++];
    const QPointer<LaunchPreparation> guard(this);
    if (!transition(taskId, PreparationPhase::ProbingJava)) {
        if (guard && guard->isCurrent(taskId, PreparationPhase::Cancelling)) {
            guard->operationId_.clear();
            static_cast<void>(guard->transition(taskId, PreparationPhase::Cancelled));
        }
        return;
    }
    probe_.probe(operationId, executable, [guard, taskId, operationId](JavaProbeResult result) {
        if (guard) { guard->javaArrived(taskId, operationId, std::move(result)); }
    });
}

void LaunchPreparation::javaArrived(const QString& taskId, const QString& operationId, JavaProbeResult result)
{
    if (shuttingDown_ || state_.taskId != taskId || operationId_ != operationId) { return; }
    if (state_.phase == PreparationPhase::Cancelling) {
        operationId_.clear();
        static_cast<void>(transition(taskId, PreparationPhase::Cancelled));
        return;
    }
    if (state_.phase != PreparationPhase::ProbingJava) { return; }
    operationId_.clear();
    if (const auto error = std::get_if<JavaProbeError>(&result)) {
        lastProbeError_ = *error;
        nextJava(taskId);
        return;
    }
    const auto runtime = std::get<core::JavaRuntime>(std::move(result));
    if (runtime.executable != candidates_[candidateIndex_ - 1]) {
        lastProbeError_ = JavaProbeError::InvalidOutput;
        nextJava(taskId);
        return;
    }
    input_->context.runtimes.push_back(runtime);
    if (input_->manualJava) { input_->context.manualRuntime = runtime; }
    const auto selected = core::selectJavaRuntime(input_->version.java, input_->context.runtimes,
                                                 input_->context.manualRuntime);
    if (std::holds_alternative<core::JavaRuntime>(selected)) { makePlan(taskId); }
    else { nextJava(taskId); }
}

void LaunchPreparation::makePlan(const QString& taskId)
{
    if (!transition(taskId, PreparationPhase::Planning)) { return; }
    auto result = core::buildLaunchPlan(input_->version, input_->context);
    if (const auto error = std::get_if<core::PlanError>(&result)) {
        fail(taskId, {PreparationErrorCode::InvalidPlan, {}, *error});
        return;
    }
    auto plan = std::get<core::LaunchPlan>(std::move(result));
    if (transition(taskId, PreparationPhase::Prepared)) { emit prepared(taskId, plan); }
}

void LaunchPreparation::fail(const QString& taskId, PreparationError error)
{
    operationId_.clear();
    state_.error = std::move(error);
    static_cast<void>(transition(taskId, PreparationPhase::Failed));
}

} // namespace hustmcl::application
