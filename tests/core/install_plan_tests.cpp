#include <hustmcl/core/install_plan.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

using namespace hustmcl::core;
namespace {
void require(bool condition, std::string_view scenario)
{
    if (!condition) { throw std::runtime_error(std::string(scenario)); }
}
VersionMetadata version()
{
    const DownloadDescriptor file{"https://example.org/file", std::string(40, 'a'), 123};
    VersionMetadata v;
    v.id = "test-21"; v.type = "release"; v.java = {21};
    v.mainClass = "net.minecraft.client.main.Main";
    v.client = v.loggingFile = file;
    v.assetIndex = {"https://example.org/index", std::string(40, 'a'), 100};
    v.assetIndexId = "17"; v.loggingId = "client.xml";
    v.loggingArgument = "-Dlog4j.configurationFile=${path}";
    v.libraries = {{"test:base:1", "test/base.jar", file, {}}};
    v.jvmArguments = {{{"-cp", "${classpath}"}, {}}};
    v.gameArguments = {{{"--accessToken", "${auth_access_token}"}, {}}};
    return v;
}
AssetIndex index()
{
    return {std::string(40, 'a'), 100, {
        {"sounds/b.ogg", std::string(40, 'b'), 20}, {"lang/a.json", std::string(40, 'a'), 10},
        {"sounds/alias.ogg", std::string(40, 'b'), 20}, {"empty", std::string(40, 'c'), 0}}};
}
std::vector<std::string> describe(const std::vector<PlannedFile>& files)
{
    std::vector<std::string> result;
    for (const auto& f : files) {
        result.push_back(f.relativePath.generic_string() + '\t' + f.download.url + '\t' + f.download.sha1
                         + '\t' + std::to_string(f.download.size) + '\t' + std::to_string(static_cast<int>(f.role)));
    }
    return result;
}
void expectError(const InstallFilePlanResult& result, PlanErrorCode code, std::string_view scenario)
{
    require(std::holds_alternative<PlanError>(result) && std::get<PlanError>(result).code == code, scenario);
    require(std::get<PlanError>(result).field.find("private") == std::string::npos, "error is schema-only");
}
void runTests()
{
    const auto v = version();
    const auto assets = index();
    const auto result = buildInstallFilePlan(v, assets);
    require(std::holds_alternative<InstallFilePlan>(result), "plan needs no Java runtime, account, data root or I/O");
    const auto& plan = std::get<InstallFilePlan>(result);
    require(plan.versionId == "test-21" && plan.java.majorVersion == 21 && plan.files.size() == 7
            && plan.logicalAssetCount == 4 && plan.uniqueAssetCount == 3 && plan.totalBytes == 499,
            "base files plus unique content including zero bytes");
    const std::vector<std::filesystem::path> expectedPaths{
        "libraries/test/base.jar", "versions/test-21/test-21.jar", "assets/indexes/17.json", "assets/log_configs/client.xml",
        "assets/objects/aa/" + std::string(40, 'a'), "assets/objects/bb/" + std::string(40, 'b'), "assets/objects/cc/" + std::string(40, 'c')};
    for (std::size_t i = 0; i < plan.files.size(); ++i) {
        require(plan.files[i].relativePath == expectedPaths[i], "full deterministic destination sequence");
    }
    require(plan.files[4].download.url == "https://resources.download.minecraft.net/aa/" + std::string(40, 'a')
            && plan.files[5].download.sha1 == std::string(40, 'b') && plan.files[6].download.size == 0
            && plan.files[6].role == FileRole::AssetObject, "fixed official content URLs, hashes, sizes and roles");
    auto changed = assets;
    std::reverse(changed.objects.begin(), changed.objects.end());
    require(describe(std::get<InstallFilePlan>(buildInstallFilePlan(v, changed)).files) == describe(plan.files),
            "input object order does not affect plan");
    changed.objects.clear();
    require(std::get<InstallFilePlan>(buildInstallFilePlan(v, changed)).files.size() == 4, "empty object map remains base-only plan");
    changed = assets; changed.objects[0].logicalName = "CON";
    require(std::holds_alternative<InstallFilePlan>(buildInstallFilePlan(v, changed)), "logical identifier never becomes a Windows filename");
    changed = assets; changed.objects[2].size = 21;
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "same content hash with conflicting size rejected");
    changed = assets; changed.objects[2].logicalName = changed.objects[0].logicalName;
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "duplicate logical names rejected");
    for (const auto* name : {"../private", "/private", "a//b", "a/./b", "a/../b", "a\\b", "C:private", "a/", "a\nprivate"}) {
        changed = assets; changed.objects[0].logicalName = name;
        expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "unsupported resource identifier rejected");
    }
    for (const auto& hash : {std::string(39, 'a'), std::string(40, 'A'), std::string(40, 'g'), std::string("../private")}) {
        changed = assets; changed.objects[0].sha1 = hash;
        expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "hash cannot influence destination structure");
    }
    changed = assets; changed.sourceSha1 = std::string(40, 'b');
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "wrong source hash binding");
    changed = assets; changed.sourceSize = 101;
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "wrong source size binding");
    changed = assets; changed.sourceSize = MaximumAssetIndexBytes + 1;
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "raw index upper bound");
    changed = assets; changed.objects.resize(MaximumAssetObjects + 1);
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::InvalidMetadata, "object count upper bound");
    changed = assets; changed.objects[1].size = std::numeric_limits<std::uint64_t>::max();
    expectError(buildInstallFilePlan(v, changed), PlanErrorCode::SizeOverflow, "byte sum cannot wrap");

    auto rules = v;
    VersionRule linux; linux.os = OperatingSystem::Linux;
    auto extra = rules.libraries[0]; extra.relativePath = "test/linux.jar"; extra.rules = std::vector{linux};
    rules.libraries.push_back(extra);
    VersionRule custom; custom.features = {{"has_custom_resolution", true}};
    extra.relativePath = "test/custom.jar"; extra.rules = std::vector{custom}; rules.libraries.push_back(extra);
    for (bool customResolution : {false, true}) {
        LaunchContext context;
        context.dataRoot = L"D:\\华科 data"; context.gameDirectory = L"D:\\game"; context.nativesDirectory = L"D:\\natives";
        context.runtimes = {{L"C:\\Java 21\\bin\\java.exe", 21, RuntimeArchitecture::X64}};
        if (customResolution) { context.resolution = Resolution{1280, 720}; }
        const auto launch = std::get<LaunchPlan>(buildLaunchPlan(rules, context));
        auto install = std::get<InstallFilePlan>(buildInstallFilePlan(rules, assets, {{"has_custom_resolution", customResolution}}));
        install.files.resize(launch.files.size());
        require(describe(install.files) == describe(launch.files) && launch.files.size() == (customResolution ? 5U : 4U),
                "install and launch use identical ordered platform/feature selection");
    }
    rules = v; rules.libraries[0].relativePath = "../private.jar";
    expectError(buildInstallFilePlan(rules, assets), PlanErrorCode::InvalidMetadata, "shared physical path validation");
    rules = v; rules.libraries.push_back(rules.libraries[0]); rules.libraries.back().relativePath = "TEST/BASE.jar";
    expectError(buildInstallFilePlan(rules, assets), PlanErrorCode::FilePathCollision, "shared Windows case collision validation");
    for (bool reverse : {false, true}) {
        rules = v; auto child = rules.libraries[0]; child.relativePath = "TEST/BASE.jar/child.jar";
        rules.libraries.push_back(child);
        if (reverse) { std::reverse(rules.libraries.begin(), rules.libraries.end()); }
        expectError(buildInstallFilePlan(rules, assets), PlanErrorCode::FilePathCollision,
                    "file cannot also be directory, independent of metadata order");
    }
}
} // namespace
int main()
{
    try { runTests(); std::cout << "PASS installation file-plan rules\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
