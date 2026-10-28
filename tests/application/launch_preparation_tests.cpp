#include <hustmcl/application/launch_preparation.h>

#include <QCoreApplication>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

using namespace hustmcl::application;
using namespace hustmcl::core;
namespace {
void require(bool condition, std::string_view scenario)
{
    if (!condition) { throw std::runtime_error(std::string(scenario)); }
}

LaunchInput input()
{
    const DownloadDescriptor file{"https://example.org/file", std::string(40, 'a'), 123};
    LaunchInput result;
    auto& v = result.version;
    v.id = "test-21";
    v.type = "release";
    v.mainClass = "net.minecraft.client.main.Main";
    v.java = {21};
    v.client = v.assetIndex = v.loggingFile = file;
    v.assetIndexId = "17";
    v.loggingId = "client.xml";
    v.loggingArgument = "-Dlog4j.configurationFile=${path}";
    v.libraries = {{"test:base:1", "test/base.jar", file, {}}};
    v.jvmArguments = {{{"-cp", "${classpath}"}, {}}};
    v.gameArguments = {{{"--accessToken", "${auth_access_token}"}, {}}};
    result.context.dataRoot = L"D:\\华科 测试\\data";
    result.context.gameDirectory = L"D:\\华科 测试\\game";
    result.context.nativesDirectory = L"D:\\华科 测试\\natives";
    result.javaCandidates = {L"C:\\Java 21\\bin\\java.exe"};
    return result;
}

struct Source final : LaunchInputSource {
    std::vector<Completion> calls;
    void load(const QString&, Completion complete) override { calls.push_back(std::move(complete)); }
    void reply(std::size_t index, LaunchInputResult value) { auto callback = calls.at(index); callback(std::move(value)); }
};
struct Probe final : JavaRuntimeProbe {
    struct Call { QString id; std::filesystem::path path; Completion callback; };
    std::vector<Call> calls;
    std::vector<QString> cancellations;
    void probe(const QString& id, const std::filesystem::path& path, Completion complete) override
    { calls.push_back({id, path, std::move(complete)}); }
    void cancel(const QString& id) override { cancellations.push_back(id); }
    void reply(std::size_t index, JavaProbeResult value) { auto callback = calls.at(index).callback; callback(std::move(value)); }
    void runtime(std::size_t index, int major = 21, RuntimeArchitecture arch = RuntimeArchitecture::X64)
    { reply(index, JavaRuntime{calls.at(index).path, major, arch}); }
};

void runTests()
{
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        std::optional<LaunchPlan> plan;
        QObject::connect(&useCase, &LaunchPreparation::prepared, &useCase,
                         [&](const QString&, const LaunchPlan& value) { plan = value; });
        require(!useCase.start(" "), "empty instance rejected");
        require(useCase.start("instance"), "begin preparation");
        require(!useCase.start("instance"), "active task rejects duplicate start");
        source.reply(0, input());
        require(!plan && useCase.snapshot().phase == PreparationPhase::ProbingJava, "wait for actual Java result");
        probe.runtime(0);
        require(plan && plan->runtime.executable == probe.calls[0].path && plan->classpath.size() == 2,
                "actual probe drives core plan");
        require(useCase.snapshot().phase == PreparationPhase::Prepared && !useCase.snapshot().cancelAvailable,
                "prepared is terminal preparation state");
        require(std::holds_alternative<SessionField>(plan->arguments.back().front()), "credentials remain typed references");
    }
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        require(useCase.start("instance"), "input failure start");
        source.reply(0, PreparationError{PreparationErrorCode::InputUnavailable, {}, {}});
        require(probe.calls.empty() && useCase.snapshot().error->code == PreparationErrorCode::InputUnavailable,
                "input failure never invokes probe");
        require(useCase.start("instance"), "retry after failure");
        auto value = input();
        value.javaCandidates.clear();
        value.context.runtimes = {{L"C:\\unprobed\\java.exe", 21, RuntimeArchitecture::X64}};
        value.context.manualRuntime = value.context.runtimes[0];
        source.reply(1, value);
        require(probe.calls.empty() && useCase.snapshot().error->code == PreparationErrorCode::JavaUnavailable,
                "unprobed runtime descriptors never satisfy Java requirement");
        require(useCase.start("instance"), "invalid requirement start");
        value.version.java.majorVersion = 0;
        source.reply(2, value);
        require(probe.calls.empty() && useCase.snapshot().error->code == PreparationErrorCode::InvalidInput,
                "invalid requirement stops before external operation");
    }
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        require(useCase.start("instance"), "candidate selection start");
        auto value = input();
        value.javaCandidates = {L"C:\\a\\java.exe", L"C:\\b\\java.exe", L"C:\\c\\java.exe", L"C:\\d\\java.exe"};
        source.reply(0, value);
        probe.reply(0, JavaProbeError::FailedToStart);
        probe.runtime(1, 17);
        probe.runtime(2, 21, RuntimeArchitecture::X86);
        require(probe.calls.size() == 4, "failed, wrong-major and x86 candidates skipped in priority order");
        probe.runtime(3);
        require(useCase.snapshot().phase == PreparationPhase::Prepared, "exact major and x64 accepted");
        require(useCase.start("instance"), "manual selection start");
        value.manualJava = L"C:\\manual\\java.exe";
        source.reply(1, value);
        require(probe.calls[4].path == *value.manualJava, "manual path takes precedence");
        probe.runtime(4, 17);
        require(probe.calls.size() == 5 && useCase.snapshot().error->code == PreparationErrorCode::InvalidManualJava,
                "invalid manual runtime never falls back");
    }
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        require(useCase.start("instance"), "cancellation start");
        const auto oldTask = useCase.snapshot().taskId;
        require(!useCase.cancel("wrong-task"), "wrong task cancellation ignored");
        require(useCase.cancel(oldTask), "cancel read-only input wait");
        require(useCase.start("instance"), "restart with new task");
        const auto newTask = useCase.snapshot().taskId;
        require(newTask != oldTask, "retry changes task identity");
        source.reply(0, input());
        require(probe.calls.empty(), "old input callback ignored");
        source.reply(1, input());
        require(useCase.cancel(newTask), "request active process cancellation");
        require(useCase.snapshot().phase == PreparationPhase::Cancelling && !useCase.start("instance"),
                "cancellation not complete before process acknowledgement");
        require(probe.cancellations.size() == 1 && probe.cancellations[0] == probe.calls[0].id,
                "cancel exact operation");
        probe.runtime(0);
        require(useCase.snapshot().phase == PreparationPhase::Cancelled, "late success during cancellation cannot prepare");
        require(useCase.start("instance"), "restart after acknowledgement");
        source.reply(2, input());
        probe.runtime(0);
        require(useCase.snapshot().phase == PreparationPhase::ProbingJava, "old probe callback ignored in new task");
        probe.reply(1, JavaRuntime{L"C:\\different\\java.exe", 21, RuntimeArchitecture::X64});
        require(useCase.snapshot().error->probeError == JavaProbeError::InvalidOutput, "mismatched result path rejected");
    }
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        QObject::connect(&useCase, &LaunchPreparation::changed, &useCase, [&](const PreparationSnapshot& state) {
            if (state.phase == PreparationPhase::ProbingJava) { require(useCase.cancel(state.taskId), "observer cancels"); }
        });
        require(useCase.start("instance"), "reentrant cancellation start");
        source.reply(0, input());
        require(probe.calls.empty() && useCase.snapshot().phase == PreparationPhase::Cancelled,
                "cancel from observer before process start does not leave phantom active process");
    }
    {
        Source source; Probe probe;
        auto useCase = std::make_unique<LaunchPreparation>(source, probe);
        require(useCase->start("instance"), "destruction start");
        source.reply(0, input());
        useCase.reset();
        require(probe.cancellations.size() == 1, "destruction cancels owned operation");
        probe.runtime(0);
        source.reply(0, input()); // Must remain safe after receiver destruction.
    }
    {
        Source source; Probe probe;
        auto useCase = std::make_unique<LaunchPreparation>(source, probe);
        QObject::connect(useCase.get(), &LaunchPreparation::changed, [&](const PreparationSnapshot& state) {
            if (state.phase == PreparationPhase::Planning) { useCase.reset(); }
        });
        require(useCase->start("instance"), "observer destruction start");
        source.reply(0, input());
        probe.runtime(0);
        require(!useCase, "observer destruction aborts plan safely");
    }
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        require(useCase.start("instance"), "invalid plan start");
        auto value = input(); value.context.maximumMemoryMiB = 0;
        source.reply(0, value); probe.runtime(0);
        require(useCase.snapshot().error->code == PreparationErrorCode::InvalidPlan
                && useCase.snapshot().error->planError->code == PlanErrorCode::InvalidMemory,
                "core plan failure remains stable application failure");
    }
    {
        Source source; Probe probe; LaunchPreparation useCase(source, probe);
        std::vector<std::string> xmx;
        QObject::connect(&useCase, &LaunchPreparation::prepared, [&](const QString&, const hustmcl::core::LaunchPlan& plan) {
            for (const auto& argument : plan.arguments) for (const auto& part : argument)
                if (const auto text = std::get_if<std::string>(&part); text && text->rfind("-Xmx", 0) == 0) xmx.push_back(*text);
        });
        useCase.setMemoryMb(4096);
        require(useCase.start("instance"), "memory override start");
        useCase.setMemoryMb(8192); // changes after start must not leak into the running task
        source.reply(0, input()); probe.runtime(0);
        require(xmx.size() == 1 && xmx[0] == "-Xmx4096M", "memory chosen at task start reaches the launch plan");
    }
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try { runTests(); std::cout << "Launch preparation behavior tests passed\n"; return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
