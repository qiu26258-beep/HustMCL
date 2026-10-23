#include <hustmcl/core/launch_plan.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace hustmcl::core;
namespace {

void require(bool condition, std::string_view scenario)
{
    if (!condition) { throw std::runtime_error(std::string(scenario)); }
}

std::string utf8(const std::filesystem::path& path)
{
    auto preferred = path;
    preferred.make_preferred();
    const auto value = preferred.u8string();
    return {value.begin(), value.end()};
}

VersionMetadata metadata()
{
    const DownloadDescriptor file{"https://example.org/file", std::string(40, 'a'), 123};
    VersionMetadata version;
    version.id = "test-21";
    version.type = "release";
    version.mainClass = "net.minecraft.client.main.Main";
    version.java = {21};
    version.client = file;
    version.assetIndexId = "17";
    version.assetIndex = file;
    version.loggingId = "client.xml";
    version.loggingFile = file;
    version.loggingArgument = "-Dlog4j.configurationFile=${path}";
    version.libraries = {{"test:base:1", "test/base.jar", file, std::nullopt}};
    version.jvmArguments = {{{"-Djava.library.path=${natives_directory}", "-cp", "${classpath}"}, std::nullopt}};
    version.gameArguments = {{{"--username", "${auth_player_name}", "--uuid", "${auth_uuid}",
        "--accessToken", "${auth_access_token}", "--clientId", "${clientid}", "--xuid", "${auth_xuid}",
        "--gameDir", "${game_directory}"}, std::nullopt}};
    return version;
}

LaunchContext context()
{
    LaunchContext result;
    result.dataRoot = L"D:\\华科 启动器\\data";
    result.gameDirectory = L"E:\\游戏 存档\\instance\\game";
    result.nativesDirectory = result.dataRoot / L"natives";
    result.runtimes = {{L"C:\\Java 21\\bin\\java.exe", 21, RuntimeArchitecture::X64}};
    return result;
}

void expectError(const LaunchPlanResult& result, PlanErrorCode expected, std::string_view scenario)
{
    require(std::holds_alternative<PlanError>(result) && std::get<PlanError>(result).code == expected, scenario);
}

std::vector<std::string> describe(const LaunchPlan& plan)
{
    std::vector<std::string> result;
    for (const auto& argument : plan.arguments) {
        std::string value;
        for (const auto& part : argument) {
            if (const auto literal = std::get_if<std::string>(&part)) { value += *literal; }
            else { value += "<session:" + std::to_string(static_cast<int>(std::get<SessionField>(part))) + ">"; }
        }
        result.push_back(std::move(value));
    }
    return result;
}

void runTests()
{
    const auto version = metadata();
    const auto environment = context();
    const auto result = buildLaunchPlan(version, environment);
    require(std::holds_alternative<LaunchPlan>(result), "valid normalized plan");
    const auto& plan = std::get<LaunchPlan>(result);
    const auto library = environment.dataRoot / "libraries/test/base.jar";
    const auto client = environment.dataRoot / "versions/test-21/test-21.jar";
    const std::vector<std::string> expected{
        "-Xmx2048M", "-Djava.library.path=" + utf8(environment.nativesDirectory), "-cp",
        utf8(library) + ";" + utf8(client),
        "-Dlog4j.configurationFile=" + utf8(environment.dataRoot / "assets/log_configs/client.xml"),
        "net.minecraft.client.main.Main", "--username", "<session:0>", "--uuid", "<session:1>",
        "--accessToken", "<session:2>", "--clientId", "<session:3>", "--xuid", "<session:4>",
        "--gameDir", utf8(environment.gameDirectory)};
    require(describe(plan) == expected, "complete argument order, UTF-8 paths, unquoted separate arguments and session slots");
    require(plan.classpath == std::vector<std::filesystem::path>{library, client}, "libraries precede client jar");
    require(plan.runtime.executable == environment.runtimes[0].executable
        && plan.workingDirectory == environment.gameDirectory, "selected Java and isolated working directory");
    require(plan.files.size() == 4 && plan.files[0].role == FileRole::Library
        && plan.files[1].relativePath == std::filesystem::path("versions/test-21/test-21.jar")
        && plan.files[2].role == FileRole::AssetIndex && plan.files[3].role == FileRole::LoggingConfig
        && plan.files[0].download.sha1 == std::string(40, 'a') && plan.files[0].download.size == 123,
        "file requirements retain role, target, digest and size");
    require(describe(std::get<LaunchPlan>(buildLaunchPlan(version, environment))) == expected,
        "repeated planning is deterministic");

    require(std::get<bool>(evaluateWindowsX64Rules(std::nullopt)), "no rules allows");
    require(!std::get<bool>(evaluateWindowsX64Rules(std::vector<VersionRule>{})), "empty explicit rules denies");
    VersionRule allow;
    VersionRule deny; deny.action = RuleAction::Disallow;
    require(!std::get<bool>(evaluateWindowsX64Rules(std::vector{allow, deny})), "last matching disallow wins");
    require(std::get<bool>(evaluateWindowsX64Rules(std::vector{deny, allow})), "last matching allow wins");
    VersionRule linuxOnly; linuxOnly.os = OperatingSystem::Linux;
    VersionRule x86Only; x86Only.architecture = RuntimeArchitecture::X86;
    require(!std::get<bool>(evaluateWindowsX64Rules(std::vector{linuxOnly, x86Only})), "exclude Linux and x86 rules");
    VersionRule falseFeature; falseFeature.features = {{"is_demo_user", false}};
    require(std::get<bool>(evaluateWindowsX64Rules(std::vector{falseFeature})), "missing feature defaults to false");
    falseFeature.features = {{"future_feature", true}};
    require(std::holds_alternative<PlanError>(evaluateWindowsX64Rules(std::vector{falseFeature})),
        "unknown rule semantics fail closed");

    auto changed = version;
    changed.libraries[0].rules = std::vector{linuxOnly};
    require(std::get<LaunchPlan>(buildLaunchPlan(changed, environment)).classpath.size() == 1,
        "excluded libraries do not enter classpath or file requirements");
    VersionRule custom; custom.features = {{"has_custom_resolution", true}};
    changed = version;
    changed.gameArguments.push_back({{"--width", "${resolution_width}", "--height", "${resolution_height}"}, std::vector{custom}});
    require(describe(std::get<LaunchPlan>(buildLaunchPlan(changed, environment))) == expected, "disabled feature omits whole argument group");
    auto resized = environment; resized.resolution = Resolution{1280, 720};
    const auto resizedArgs = describe(std::get<LaunchPlan>(buildLaunchPlan(changed, resized)));
    require(std::vector<std::string>(resizedArgs.end() - 4, resizedArgs.end())
        == std::vector<std::string>{"--width", "1280", "--height", "720"}, "enabled resolution expands selected values");

    auto missingJava = environment; missingJava.runtimes.clear();
    expectError(buildLaunchPlan(version, missingJava), PlanErrorCode::NoCompatibleJava, "missing Java");
    missingJava.runtimes = {{L"C:\\java.exe", 17, RuntimeArchitecture::X64}};
    expectError(buildLaunchPlan(version, missingJava), PlanErrorCode::NoCompatibleJava, "wrong Java major");
    missingJava = environment; missingJava.manualRuntime = JavaRuntime{L"C:\\java.exe", 21, RuntimeArchitecture::X86};
    expectError(buildLaunchPlan(version, missingJava), PlanErrorCode::InvalidManualJava, "invalid manual choice never falls back");
    for (const auto* path : {L"relative", L"D:\\data;other", L"D:\\a\\..\\b", L"D:\\con.txt", L"D:\\CONOUT$",
                             L"\\\\server\\share", L"\\\\?\\C:\\data"}) {
        auto invalid = environment; invalid.dataRoot = path;
        expectError(buildLaunchPlan(version, invalid), PlanErrorCode::InvalidPaths, "reject unsafe or unsupported absolute data roots");
    }
    for (const auto* path : {"../escape.jar", "C:/escape.jar", "bad/CON.jar", "bad/file..", "bad/file:stream", "bad/a;b.jar"}) {
        changed = version; changed.libraries[0].relativePath = path;
        expectError(buildLaunchPlan(changed, environment), PlanErrorCode::InvalidMetadata, "reject traversal, devices, aliases and streams");
    }
    changed = version;
    changed.libraries[0].relativePath = "net/fabricmc/sponge-mixin/0.17.5+mixin.0.8.7/sponge-mixin-0.17.5+mixin.0.8.7.jar";
    require(std::holds_alternative<LaunchPlan>(buildLaunchPlan(changed, environment)), "verified Maven plus version is a safe relative library filename");
    changed = version; changed.libraries.push_back(changed.libraries[0]);
    changed.libraries.back().relativePath = "TEST/BASE.jar";
    expectError(buildLaunchPlan(changed, environment), PlanErrorCode::FilePathCollision, "Windows case-insensitive target collision");
    changed = version; changed.id = "../other";
    expectError(buildLaunchPlan(changed, environment), PlanErrorCode::InvalidMetadata, "unsafe version ID");
    for (const auto* name : {".Main", "net..Main", "net.1Main", "net.Main."}) {
        changed = version; changed.mainClass = name;
        expectError(buildLaunchPlan(changed, environment), PlanErrorCode::InvalidMetadata, "invalid Java main class name");
    }
    changed = version; changed.client.sha1 = "bad";
    expectError(buildLaunchPlan(changed, environment), PlanErrorCode::InvalidMetadata, "invalid required file digest");
    changed = version; changed.gameArguments[0].values.push_back("${unrecognized}");
    expectError(buildLaunchPlan(changed, environment), PlanErrorCode::UnknownPlaceholder, "unknown active placeholder");
    changed.gameArguments[0].values.back() = "${unterminated";
    expectError(buildLaunchPlan(changed, environment), PlanErrorCode::UnknownPlaceholder, "unterminated template");
    auto invalidMemory = environment; invalidMemory.maximumMemoryMiB = 0;
    expectError(buildLaunchPlan(version, invalidMemory), PlanErrorCode::InvalidMemory, "invalid memory bound");
    auto literalPath = environment; literalPath.gameDirectory = L"D:\\literal ${auth_access_token}\\game";
    const auto literalPlan = std::get<LaunchPlan>(buildLaunchPlan(version, literalPath));
    require(literalPlan.arguments.back().size() == 1
        && std::get<std::string>(literalPlan.arguments.back()[0]) == utf8(literalPath.gameDirectory),
        "replacement values are literal, never recursively parsed into credentials");
}

} // namespace
int main()
{
    try { runTests(); std::cout << "PASS launch plan and Windows x64 rule scenarios\n"; return 0; }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
