#include <hustmcl/services/java_probe.h>
#include <hustmcl/services/launch_input_source.h>
#include <hustmcl/services/install_plan_input.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

using namespace hustmcl::application;
using namespace hustmcl::core;
using namespace hustmcl::services;
namespace {
void require(bool condition, std::string_view scenario)
{
    if (!condition) { throw std::runtime_error(std::string(scenario)); }
}
template<class Result, class Begin>
Result awaitResult(Begin begin)
{
    QEventLoop loop;
    QTimer watchdog;
    watchdog.setSingleShot(true);
    std::optional<Result> result;
    QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
    watchdog.start(10000);
    begin([&](Result value) { require(!result, "completion delivered once"); result = std::move(value); loop.quit(); });
    if (!result) { loop.exec(); }
    require(result.has_value(), "asynchronous service completed before watchdog");
    return std::move(*result);
}

void runTests(const QString& fixture)
{
    QElapsedTimer elapsed;
    elapsed.start();
    const auto progress = [&](const char* stage) { std::cerr << elapsed.elapsed() << " ms: " << stage << '\n'; };
    QTemporaryDir temporary(QCoreApplication::applicationDirPath() + QStringLiteral("/java-中文 路径-XXXXXX"));
    require(temporary.isValid(), "create owned Unicode test directory");
    const auto path = temporary.path() + QStringLiteral("/java 探测.exe");
    require(QFile::copy(fixture, path), "copy owned subprocess fixture");
    const std::filesystem::path executable(path.toStdWString());
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) {
        qputenv(key, "must-be-removed-from-child");
    }
    auto probeResult = [&](const char* mode, int timeout = 3000, int outputLimit = 65536,
                           const std::filesystem::path& file = {}) {
        progress(mode);
        qputenv("HUSTMCL_TEST_PROBE_MODE", mode);
        QtJavaProbe probe(nullptr, timeout, outputLimit);
        return awaitResult<JavaProbeResult>([&](auto complete) {
            probe.probe("probe", file.empty() ? executable : file, std::move(complete));
        });
    };
    auto result = probeResult("valid");
    require(std::holds_alternative<JavaRuntime>(result), "valid fixture properties");
    const auto runtime = std::get<JavaRuntime>(result);
    require(runtime.majorVersion == 21 && runtime.architecture == RuntimeArchitecture::X64 && runtime.executable == executable,
            "version, architecture, Unicode path, fixed argv and child environment verified");
    require(std::get<JavaRuntime>(probeResult("legacy")).majorVersion == 8, "legacy 1.8 normalization");
    require(std::get<JavaRuntime>(probeResult("x86")).architecture == RuntimeArchitecture::X86, "architecture preserved for core rejection");
    for (const auto* mode : {"malformed", "duplicate"}) {
        require(std::get<JavaProbeError>(probeResult(mode)) == JavaProbeError::InvalidOutput, "malformed or ambiguous properties rejected");
    }
    require(std::get<JavaProbeError>(probeResult("nonzero")) == JavaProbeError::AbnormalExit, "nonzero exit rejected");
    require(std::get<JavaProbeError>(probeResult("delay", 100)) == JavaProbeError::TimedOut, "timeout kills subprocess");
    require(std::get<JavaProbeError>(probeResult("noisy", 3000, 1024)) == JavaProbeError::OutputLimit, "output cap enforced");
    require(std::get<JavaProbeError>(probeResult("valid", 3000, 65536, L"relative.exe")) == JavaProbeError::InvalidPath,
            "relative executable rejected");
    const auto missing = std::filesystem::path((temporary.path() + "/missing.exe").toStdWString());
    require(std::get<JavaProbeError>(probeResult("valid", 3000, 65536, missing)) == JavaProbeError::InvalidPath,
            "missing executable rejected");
    QFile broken(temporary.path() + "/invalid.exe");
    require(broken.open(QIODevice::WriteOnly), "create owned invalid exe"); broken.write("not a PE file"); broken.close();
    require(std::get<JavaProbeError>(probeResult("valid", 3000, 65536, std::filesystem::path(broken.fileName().toStdWString())))
                == JavaProbeError::FailedToStart, "malformed executable rejected before process creation");
    for (quint32 offset : {quint32(0), quint32(64), quint32(0xffffffff)}) {
        QByteArray dos(64, '\0'); dos[0] = 'M'; dos[1] = 'Z';
        for (int i = 0; i < 4; ++i) { dos[60 + i] = static_cast<char>((offset >> (8 * i)) & 0xff); }
        require(broken.open(QIODevice::WriteOnly | QIODevice::Truncate), "rewrite owned malformed DOS header");
        require(broken.write(dos) == dos.size(), "write owned malformed DOS header"); broken.close();
        require(std::get<JavaProbeError>(probeResult("valid", 3000, 65536, std::filesystem::path(broken.fileName().toStdWString())))
                    == JavaProbeError::FailedToStart, "DOS offset bounds and truncated PE rejected without execution");
    }
    {
        progress("executable held open for writing");
        QFile held(path);
        require(held.open(QIODevice::ReadWrite), "hold owned executable write handle");
        require(std::get<JavaProbeError>(probeResult("valid")) == JavaProbeError::FailedToStart,
                "real process creation failure mapped without attempting malformed PE");
    }
    {
        progress("cancel and reuse");
        qputenv("HUSTMCL_TEST_PROBE_MODE", "delay");
        QtJavaProbe probe;
        bool busySeen = false;
        result = awaitResult<JavaProbeResult>([&](auto complete) {
            probe.probe("active", executable, std::move(complete));
            probe.probe("second", executable, [&](JavaProbeResult second) {
                require(std::get<JavaProbeError>(second) == JavaProbeError::Busy, "concurrent invocation rejected");
                busySeen = true;
                probe.cancel("active");
            });
        });
        require(busySeen && std::get<JavaProbeError>(result) == JavaProbeError::Cancelled, "cancel acknowledged after child exit");
        qputenv("HUSTMCL_TEST_PROBE_MODE", "valid");
        result = awaitResult<JavaProbeResult>([&](auto complete) { probe.probe("after-cancel", executable, std::move(complete)); });
        require(std::holds_alternative<JavaRuntime>(result), "service reusable after cancellation");
    }
    {
        progress("completion destroys service");
        // A completion may destroy its service directly; no subsequent callback may touch the deleted job.
        auto probe = std::make_unique<QtJavaProbe>();
        qputenv("HUSTMCL_TEST_PROBE_MODE", "valid");
        result = awaitResult<JavaProbeResult>([&](auto complete) {
            probe->probe("self-delete", executable, [&, complete](JavaProbeResult value) {
                probe.reset(); complete(std::move(value));
            });
        });
        require(!probe && std::holds_alternative<JavaRuntime>(result), "service destruction from completion safe");
    }
    {
        progress("destroy active service");
        bool called = false;
        qputenv("HUSTMCL_TEST_PROBE_MODE", "delay");
        { QtJavaProbe probe; probe.probe("destroy-active", executable, [&](JavaProbeResult) { called = true; }); }
        QCoreApplication::processEvents();
        require(!called, "destroying active service stops child without invoking owner callback");
    }

    const auto metadata = QString::fromUtf8(HUSTMCL_FIXTURE_DIR) + "/1.21.1.json";
    progress("metadata sources");
    const QByteArray pin("1757c8b1fbfc6b638504e1f6cceb05893f7389d5");
    auto sourceResult = [&](const QString& file, const QByteArray& hash, const QString& instance = "instance") {
        MetadataFileSource source("instance", file, hash, {});
        return awaitResult<LaunchInputResult>([&](auto complete) { source.load(instance, std::move(complete)); });
    };
    auto loaded = sourceResult(metadata, pin);
    require(std::holds_alternative<LaunchInput>(loaded) && std::get<LaunchInput>(loaded).version.id == "1.21.1",
            "actual fixed file hash checked and parsed on worker");
    require(std::get<PreparationError>(sourceResult(metadata, QByteArray(40, '0'))).code == PreparationErrorCode::InvalidInput,
            "hash mismatch stops before plan");
    require(std::get<PreparationError>(sourceResult(metadata, pin, "wrong")).code == PreparationErrorCode::InputUnavailable,
            "unknown instance rejected");
    require(std::get<PreparationError>(sourceResult(temporary.path() + "/missing.json", pin)).code == PreparationErrorCode::InputUnavailable,
            "missing metadata rejected");
    const auto assetPath = QString::fromUtf8(HUSTMCL_FIXTURE_DIR) + "/17.json";
    progress("installation plan local inputs");
    auto filePlan = loadInstallFilePlan(metadata, pin, assetPath);
    require(std::holds_alternative<InstallFilePlan>(filePlan) && std::get<InstallFilePlan>(filePlan).files.size() == 3961,
            "actual local version and referenced index compose complete plan without Java");
    require(std::get<PreparationError>(loadInstallFilePlan(metadata, QByteArray(40, '0'), "missing.json")).code == PreparationErrorCode::InvalidInput,
            "bad version pin stops before reading resource index");
    require(std::get<PreparationError>(loadInstallFilePlan(metadata, pin, "relative.json")).code == PreparationErrorCode::InputUnavailable,
            "relative resource path rejected");
    require(std::get<PreparationError>(loadInstallFilePlan(metadata, pin, temporary.path() + "/missing.json")).code == PreparationErrorCode::InputUnavailable,
            "missing index cannot produce a plan");
    require(std::get<PreparationError>(loadInstallFilePlan(metadata, pin, metadata)).planError->field == "assetIndex.integrity",
            "wrong local index never satisfies version reference");
    {
        progress("destroy active metadata source");
        bool called = false;
        { MetadataFileSource source("instance", metadata, pin, {});
          source.load("instance", [&](LaunchInputResult) { called = true; }); }
        QCoreApplication::processEvents();
        require(!called, "source destruction drains worker and drops queued completion");
    }
    qunsetenv("HUSTMCL_TEST_PROBE_MODE");
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        require(app.arguments().size() == 2, "fixture path provided");
        runTests(app.arguments()[1]);
        std::cout << "Runtime subprocess and metadata input tests passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
