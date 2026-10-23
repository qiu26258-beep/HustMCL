#include <hustmcl/services/version_metadata.h>

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace hustmcl::core;
using hustmcl::services::parseVersionMetadata;
namespace {

void require(bool condition, std::string_view scenario)
{
    if (!condition) { throw std::runtime_error(std::string(scenario)); }
}

QByteArray fixture(const QString& name)
{
    QFile input(QString::fromUtf8(HUSTMCL_FIXTURE_DIR) + '/' + name);
    require(input.open(QIODevice::ReadOnly), "open pinned fixture");
    return input.readAll();
}

LaunchContext context()
{
    LaunchContext result;
    result.dataRoot = L"D:\\HustMCL 测试\\data";
    result.gameDirectory = L"E:\\游戏 存档\\instance\\game";
    result.nativesDirectory = result.dataRoot / "natives/1.21.1";
    result.runtimes = {{L"C:\\Java 21\\bin\\java.exe", 21, RuntimeArchitecture::X64}};
    return result;
}

std::string utf8(std::filesystem::path path)
{
    const auto value = path.make_preferred().u8string();
    return {value.begin(), value.end()};
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

LaunchPlanResult planned(const QJsonObject& root)
{
    const auto parsed = parseVersionMetadata(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (const auto error = std::get_if<PlanError>(&parsed)) { return *error; }
    return buildLaunchPlan(std::get<VersionMetadata>(parsed), context());
}

void expectError(const QJsonObject& root, PlanErrorCode code, std::string_view scenario)
{
    const auto result = planned(root);
    if (!std::holds_alternative<PlanError>(result)) {
        std::cerr << "Unexpected plan: " << scenario << '\n';
    } else if (std::get<PlanError>(result).code != code) {
        const auto& error = std::get<PlanError>(result);
        std::cerr << "Unexpected error " << static_cast<int>(error.code) << " at " << error.field
                  << ": " << scenario << '\n';
    }
    require(std::holds_alternative<PlanError>(result) && std::get<PlanError>(result).code == code, scenario);
    require(!std::get<PlanError>(result).field.empty(), "error has schema location");
}

QJsonObject withRule(QJsonObject root, const QJsonObject& rule)
{
    auto args = root.value("arguments").toObject();
    auto jvm = args.value("jvm").toArray();
    jvm.append(QJsonObject{{"rules", QJsonArray{rule}}, {"value", "-Dtest=true"}});
    args.insert("jvm", jvm);
    root.insert("arguments", args);
    return root;
}

void runTests()
{
    const auto bytes = fixture("1.21.1.json");
    require(bytes.size() == 38408 && QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex()
        == QByteArray("1757c8b1fbfc6b638504e1f6cceb05893f7389d5"), "original Mojang fixture byte size and SHA-1");
    const auto parsed = parseVersionMetadata(bytes);
    require(std::holds_alternative<VersionMetadata>(parsed), "parse real standalone release metadata");
    const auto& version = std::get<VersionMetadata>(parsed);
    require(version.id == "1.21.1" && version.java.majorVersion == 21 && version.libraries.size() == 97,
        "version and Java requirements come from metadata");
    const auto environment = context();
    const auto result = buildLaunchPlan(version, environment);
    require(std::holds_alternative<LaunchPlan>(result), "real fixture generates a Windows x64 plan");
    const auto& plan = std::get<LaunchPlan>(result);
    require(plan.files.size() == 73 && plan.classpath.size() == 71, "70 libraries, client, asset index and logging config");

    const auto expectedLibraries = fixture("expected-windows-libraries.txt").trimmed().split('\n');
    require(expectedLibraries.size() == 70, "reviewed Windows classpath expectation");
    std::string classpath;
    for (qsizetype i = 0; i < expectedLibraries.size(); ++i) {
        const auto relative = std::filesystem::path("libraries") / expectedLibraries[i].trimmed().toStdString();
        require(plan.files[static_cast<std::size_t>(i)].relativePath == relative
            && plan.classpath[static_cast<std::size_t>(i)] == environment.dataRoot / relative,
            "full selected library order matches reviewed expectation");
        if (!classpath.empty()) { classpath += ';'; }
        classpath += utf8(environment.dataRoot / relative);
    }
    const auto client = environment.dataRoot / "versions/1.21.1/1.21.1.jar";
    classpath += ';' + utf8(client);
    require(plan.classpath.back() == client && plan.files[70].download.sha1 == "30c73b1c5da787909b2f73340419fdf13b9def88"
        && plan.files[70].download.size == 26836906, "client requirement matches pinned metadata");
    const std::vector<std::string> expectedArguments{
        "-Xmx2048M", "-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump",
        "-Djava.library.path=" + utf8(environment.nativesDirectory), "-Djna.tmpdir=" + utf8(environment.nativesDirectory),
        "-Dorg.lwjgl.system.SharedLibraryExtractPath=" + utf8(environment.nativesDirectory),
        "-Dio.netty.native.workdir=" + utf8(environment.nativesDirectory),
        "-Dminecraft.launcher.brand=HustMCL", "-Dminecraft.launcher.version=0.1.0", "-cp", classpath,
        "-Dlog4j.configurationFile=" + utf8(environment.dataRoot / "assets/log_configs/client-1.12.xml"),
        "net.minecraft.client.main.Main", "--username", "<session:0>", "--version", "1.21.1",
        "--gameDir", utf8(environment.gameDirectory), "--assetsDir", utf8(environment.dataRoot / "assets"),
        "--assetIndex", "17", "--uuid", "<session:1>", "--accessToken", "<session:2>",
        "--clientId", "<session:3>", "--xuid", "<session:4>", "--userType", "msa", "--versionType", "release"};
    require(describe(plan) == expectedArguments, "complete real-version JVM/main/game argument vector");
    require(plan.runtime.majorVersion == 21 && plan.workingDirectory == environment.gameDirectory,
        "plan selects Java 21 and isolated game directory");

    const auto original = QJsonDocument::fromJson(bytes).object();
    for (const auto& input : {QByteArray("{"), QByteArray("[]"), QByteArray("null"), QByteArray(4 * 1024 * 1024 + 1, ' ')}) {
        require(std::holds_alternative<PlanError>(parseVersionMetadata(input)), "malformed, wrong root and oversized JSON fail safely");
    }
    for (const auto* field : {"javaVersion", "arguments", "downloads", "assetIndex", "logging", "mainClass"}) {
        auto changed = original; changed.remove(field);
        expectError(changed, PlanErrorCode::InvalidMetadata, "missing required metadata");
    }
    for (const auto* field : {"inheritsFrom", "minecraftArguments", "futureRequiredField"}) {
        auto changed = original; changed.insert(field, "untrusted-value");
        expectError(changed, PlanErrorCode::UnsupportedMetadata, "unimplemented manifest semantics rejected");
    }
    {
        auto recommended = original;
        auto args = recommended.value("arguments").toObject();
        args.insert("default-user-jvm", QJsonArray{
            QJsonObject{{"value", QJsonArray{"-Xms2G", "-Xmx4G"}}},
            QJsonObject{{"rules", QJsonArray{QJsonObject{{"action", "allow"},
                {"os", QJsonObject{{"name", "windows"}, {"versionRange", QJsonObject{{"min", "10.0.17134"}}}}}}}},
                {"value", "-XX:+UseZGC"}}});
        recommended.insert("arguments", args);
        const auto result = planned(recommended);
        require(std::holds_alternative<LaunchPlan>(result), "user JVM recommendations accepted");
        require(describe(std::get<LaunchPlan>(result)) == describe(std::get<LaunchPlan>(planned(original))),
            "explicit launcher heap policy overrides optional recommendations only");
        args.insert("default-user-jvm", "bad"); recommended.insert("arguments", args);
        expectError(recommended, PlanErrorCode::InvalidMetadata, "malformed recommendations rejected");
        args.insert("future-jvm", QJsonArray{}); recommended.insert("arguments", args);
        expectError(recommended, PlanErrorCode::UnsupportedMetadata, "unknown JVM sections still rejected");
    }
    auto changed = original; changed.insert("minimumLauncherVersion", 22);
    expectError(changed, PlanErrorCode::UnsupportedMetadata, "future minimum launcher profile rejected");
    changed = original; changed.insert("type", "snapshot");
    expectError(changed, PlanErrorCode::UnsupportedMetadata, "snapshot outside first profile");
    changed = original; auto java = changed.value("javaVersion").toObject(); java.insert("majorVersion", 21.5);
    changed.insert("javaVersion", java);
    expectError(changed, PlanErrorCode::InvalidMetadata, "fractional Java major rejected");

    expectError(withRule(original, {{"action", "future-action"}}), PlanErrorCode::UnsupportedMetadata, "unknown action");
    expectError(withRule(original, {{"action", "allow"}, {"os", QJsonObject{{"name", "windows"}, {"version", ".*"}}}}),
        PlanErrorCode::UnsupportedMetadata, "unimplemented OS regex rejected");
    expectError(withRule(original, {{"action", "allow"}, {"features", QJsonObject{{"future_feature", true}}}}),
        PlanErrorCode::UnsupportedMetadata, "unknown feature is not silently defaulted");
    expectError(withRule(original, {{"action", "allow"}, {"features", QJsonObject{{"is_demo_user", "false"}}}}),
        PlanErrorCode::InvalidMetadata, "feature must be a JSON boolean");
    expectError(withRule(original, {{"action", "allow"}, {"os", QJsonObject{{"arch", "mips"}}}}),
        PlanErrorCode::UnsupportedMetadata, "unknown architecture");

    for (const auto* path : {"../escape.jar", "D:/absolute.jar", "bad/NUL.jar", "bad/jar:stream"}) {
        changed = original;
        auto libraries = changed.value("libraries").toArray();
        auto library = libraries[1].toObject(); // First fixture entry is macOS-only; mutate a selected library.
        auto downloads = library.value("downloads").toObject();
        auto artifact = downloads.value("artifact").toObject(); artifact.insert("path", path);
        downloads.insert("artifact", artifact); library.insert("downloads", downloads); libraries[1] = library;
        changed.insert("libraries", libraries);
        expectError(changed, PlanErrorCode::InvalidMetadata, std::string("untrusted artifact path: ") + path);
    }
    for (const auto& value : {QJsonValue(-1), QJsonValue(12.5), QJsonValue("123"), QJsonValue(1e30)}) {
        changed = original;
        auto downloads = changed.value("downloads").toObject(); auto clientDownload = downloads.value("client").toObject();
        clientDownload.insert("size", value); downloads.insert("client", clientDownload); changed.insert("downloads", downloads);
        expectError(changed, PlanErrorCode::InvalidMetadata, "invalid artifact byte size");
    }
    changed = original;
    auto badLibraries = changed.value("libraries").toArray();
    auto badLibrary = badLibraries[1].toObject();
    auto badDownloads = badLibrary.value("downloads").toObject();
    auto badArtifact = badDownloads.value("artifact").toObject();
    badArtifact.insert("path", QString(QChar(0xd800)));
    badDownloads.insert("artifact", badArtifact); badLibrary.insert("downloads", badDownloads);
    badLibraries[1] = badLibrary; changed.insert("libraries", badLibraries);
    expectError(changed, PlanErrorCode::InvalidMetadata, "unpaired Unicode surrogate cannot throw during path conversion");
    for (const auto* address : {"http://example.org/file", "https://user:password@example.org/file", "https://example.org/file#fragment"}) {
        changed = original;
        auto downloads = changed.value("downloads").toObject(); auto clientDownload = downloads.value("client").toObject();
        clientDownload.insert("url", address); downloads.insert("client", clientDownload); changed.insert("downloads", downloads);
        expectError(changed, PlanErrorCode::InvalidMetadata, "URLs require HTTPS and contain no credentials or fragments");
    }
    changed = original;
    auto args = changed.value("arguments").toObject(); auto game = args.value("game").toArray();
    game.append("${private-untrusted-name}"); args.insert("game", game); changed.insert("arguments", args);
    const auto unknown = planned(changed);
    require(std::holds_alternative<PlanError>(unknown)
        && std::get<PlanError>(unknown).code == PlanErrorCode::UnknownPlaceholder
        && std::get<PlanError>(unknown).field.find("private-untrusted-name") == std::string::npos,
        "unknown template error does not echo input values");
}

} // namespace
int main()
{
    try { runTests(); std::cout << "PASS pinned Mojang metadata and invalid-input scenarios\n"; return 0; }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
