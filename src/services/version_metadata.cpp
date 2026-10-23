#include <hustmcl/services/version_metadata.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QUrl>

#include <cmath>
#include <initializer_list>
#include <utility>

namespace hustmcl::services {
namespace {

using namespace core;
struct ParseFailure { PlanError error; };

[[noreturn]] void invalid(const char* field)
{
    throw ParseFailure{{PlanErrorCode::InvalidMetadata, field}};
}

[[noreturn]] void unsupported(const char* field)
{
    throw ParseFailure{{PlanErrorCode::UnsupportedMetadata, field}};
}

QJsonObject object(const QJsonValue& value, const char* field)
{
    if (!value.isObject()) { invalid(field); }
    return value.toObject();
}

void keys(const QJsonObject& value, std::initializer_list<const char*> allowed, const char* field)
{
    for (auto it = value.begin(); it != value.end(); ++it) {
        bool recognized = false;
        for (const auto* key : allowed) {
            if (it.key() == QLatin1String(key)) { recognized = true; break; }
        }
        if (!recognized) { unsupported(field); }
    }
}

QString text(const QJsonValue& value, const char* field)
{
    if (!value.isString() || value.toString().isEmpty()) { invalid(field); }
    const auto result = value.toString();
    for (qsizetype i = 0; i < result.size(); ++i) {
        const auto ch = result[i];
        if (ch.unicode() < 32 || ch.unicode() == 127) { invalid(field); }
        // Reject unpaired UTF-16 before creating a native Windows filesystem path.
        if (ch.isHighSurrogate()) {
            if (i + 1 >= result.size() || !result[i + 1].isLowSurrogate()) { invalid(field); }
            ++i;
        } else if (ch.isLowSurrogate()) { invalid(field); }
    }
    return result;
}

qint64 integer(const QJsonValue& value, const char* field)
{
    if (!value.isDouble()) { invalid(field); }
    const double result = value.toDouble();
    if (!std::isfinite(result) || result <= 0 || result > 9007199254740991.0
        || std::floor(result) != result) { invalid(field); }
    return static_cast<qint64>(result);
}

DownloadDescriptor download(const QJsonObject& value, const char* field)
{
    const auto address = text(value.value("url"), field);
    const QUrl url(address, QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != QLatin1String("https") || url.host().isEmpty()
        || !url.userInfo().isEmpty() || url.hasFragment() || address.contains('\\')) { invalid(field); }
    const auto sha1 = text(value.value("sha1"), field).toLower();
    if (sha1.size() != 40) { invalid(field); }
    for (const auto ch : sha1) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) { invalid(field); }
    }
    return {address.toStdString(), sha1.toStdString(), static_cast<std::uint64_t>(integer(value.value("size"), field))};
}

VersionRules rules(const QJsonObject& value)
{
    if (!value.contains("rules")) { return std::nullopt; }
    if (!value.value("rules").isArray()) { invalid("rules"); }
    std::vector<VersionRule> result;
    for (const auto& entry : value.value("rules").toArray()) {
        const auto ruleObject = object(entry, "rules[]");
        keys(ruleObject, {"action", "os", "features"}, "rules[]");
        VersionRule rule;
        const auto action = text(ruleObject.value("action"), "rules.action");
        if (action == QLatin1String("allow")) { rule.action = RuleAction::Allow; }
        else if (action == QLatin1String("disallow")) { rule.action = RuleAction::Disallow; }
        else { unsupported("rules.action"); }
        if (ruleObject.contains("os")) {
            const auto os = object(ruleObject.value("os"), "rules.os");
            keys(os, {"name", "arch"}, "rules.os"); // Regex / OS-version constraints need their own verified use case.
            if (os.contains("name")) {
                const auto name = text(os.value("name"), "rules.os.name");
                if (name == QLatin1String("windows")) { rule.os = OperatingSystem::Windows; }
                else if (name == QLatin1String("linux")) { rule.os = OperatingSystem::Linux; }
                else if (name == QLatin1String("osx")) { rule.os = OperatingSystem::MacOS; }
                else { unsupported("rules.os.name"); }
            }
            if (os.contains("arch")) {
                const auto arch = text(os.value("arch"), "rules.os.arch");
                if (arch == QLatin1String("x86")) { rule.architecture = RuntimeArchitecture::X86; }
                else if (arch == QLatin1String("x86_64") || arch == QLatin1String("amd64")) {
                    rule.architecture = RuntimeArchitecture::X64;
                } else if (arch == QLatin1String("arm64") || arch == QLatin1String("aarch64")) {
                    rule.architecture = RuntimeArchitecture::Arm64;
                } else { unsupported("rules.os.arch"); }
            }
        }
        if (ruleObject.contains("features")) {
            const auto features = object(ruleObject.value("features"), "rules.features");
            for (auto it = features.begin(); it != features.end(); ++it) {
                if (!it.value().isBool()) { invalid("rules.features"); }
                rule.features.emplace(it.key().toStdString(), it.value().toBool());
            }
        }
        result.push_back(std::move(rule));
    }
    VersionRules parsed = std::move(result);
    if (const auto decision = evaluateWindowsX64Rules(parsed); std::holds_alternative<PlanError>(decision)) {
        throw ParseFailure{std::get<PlanError>(decision)};
    }
    return parsed;
}

std::vector<VersionArgument> arguments(const QJsonValue& value)
{
    if (!value.isArray() || value.toArray().isEmpty()) { invalid("arguments"); }
    std::vector<VersionArgument> result;
    for (const auto& entry : value.toArray()) {
        VersionArgument argument;
        if (entry.isString()) {
            argument.values.push_back(text(entry, "arguments.value").toStdString());
        } else {
            const auto obj = object(entry, "arguments[]");
            keys(obj, {"rules", "value"}, "arguments[]");
            if (!obj.contains("rules")) { invalid("arguments.rules"); }
            argument.rules = rules(obj);
            const auto values = obj.value("value");
            if (values.isString()) {
                argument.values.push_back(text(values, "arguments.value").toStdString());
            } else if (values.isArray() && !values.toArray().isEmpty()) {
                for (const auto& item : values.toArray()) {
                    argument.values.push_back(text(item, "arguments.value[]").toStdString());
                }
            } else { invalid("arguments.value"); }
        }
        result.push_back(std::move(argument));
    }
    return result;
}

VersionMetadata parse(const QJsonObject& root)
{
    keys(root, {"id", "type", "mainClass", "javaVersion", "downloads", "assetIndex", "assets",
        "logging", "libraries", "arguments", "minimumLauncherVersion", "complianceLevel",
        "releaseTime", "time"}, "version.fields");
    VersionMetadata result;
    result.id = text(root.value("id"), "id").toStdString();
    result.type = text(root.value("type"), "type").toStdString();
    if (result.type != "release") { unsupported("type"); }
    result.mainClass = text(root.value("mainClass"), "mainClass").toStdString();
    const auto java = object(root.value("javaVersion"), "javaVersion");
    keys(java, {"component", "majorVersion"}, "javaVersion");
    const auto major = integer(java.value("majorVersion"), "javaVersion.majorVersion");
    if (major > 1000) { unsupported("javaVersion.majorVersion"); }
    result.java = {static_cast<int>(major)};
    if (root.contains("minimumLauncherVersion")
        && integer(root.value("minimumLauncherVersion"), "minimumLauncherVersion") > 21) {
        unsupported("minimumLauncherVersion");
    }
    if (root.contains("complianceLevel") && (!root.value("complianceLevel").isDouble()
        || (root.value("complianceLevel").toDouble() != 0 && root.value("complianceLevel").toDouble() != 1))) {
        unsupported("complianceLevel");
    }
    const auto downloads = object(root.value("downloads"), "downloads");
    keys(downloads, {"client", "server", "client_mappings", "server_mappings"}, "downloads");
    const auto client = object(downloads.value("client"), "downloads.client");
    keys(client, {"url", "sha1", "size"}, "downloads.client");
    result.client = download(client, "downloads.client");
    const auto assets = object(root.value("assetIndex"), "assetIndex");
    keys(assets, {"id", "url", "sha1", "size", "totalSize"}, "assetIndex");
    result.assetIndexId = text(assets.value("id"), "assetIndex.id").toStdString();
    result.assetIndex = download(assets, "assetIndex");
    const auto logging = object(root.value("logging"), "logging");
    keys(logging, {"client"}, "logging");
    const auto loggingClient = object(logging.value("client"), "logging.client");
    keys(loggingClient, {"type", "argument", "file"}, "logging.client");
    if (text(loggingClient.value("type"), "logging.client.type") != QLatin1String("log4j2-xml")) {
        unsupported("logging.client.type");
    }
    const auto loggingFile = object(loggingClient.value("file"), "logging.client.file");
    keys(loggingFile, {"id", "url", "sha1", "size"}, "logging.client.file");
    result.loggingId = text(loggingFile.value("id"), "logging.client.file.id").toStdString();
    result.loggingFile = download(loggingFile, "logging.client.file");
    result.loggingArgument = text(loggingClient.value("argument"), "logging.client.argument").toStdString();
    if (!root.value("libraries").isArray() || root.value("libraries").toArray().isEmpty()) { invalid("libraries"); }
    for (const auto& entry : root.value("libraries").toArray()) {
        const auto library = object(entry, "libraries[]");
        keys(library, {"name", "downloads", "rules"}, "libraries[]");
        const auto libraryDownloads = object(library.value("downloads"), "libraries.downloads");
        keys(libraryDownloads, {"artifact"}, "libraries.downloads");
        const auto artifact = object(libraryDownloads.value("artifact"), "libraries.artifact");
        keys(artifact, {"path", "url", "sha1", "size"}, "libraries.artifact");
        result.libraries.push_back({text(library.value("name"), "libraries.name").toStdString(),
            std::filesystem::path(text(artifact.value("path"), "libraries.path").toStdWString()),
            download(artifact, "libraries.artifact"), rules(library)});
    }
    const auto args = object(root.value("arguments"), "arguments");
    keys(args, {"jvm", "game", "default-user-jvm"}, "arguments");
    // These are user-overridable tuning defaults, not the required JVM arguments.
    // HustMCL supplies its own bounded heap policy and uses the JVM's default GC.
    // Validate their shape without applying OS-dependent recommendations.
    if (args.contains("default-user-jvm")) {
        const auto defaults = args.value("default-user-jvm");
        if (!defaults.isArray()) { invalid("arguments.default-user-jvm"); }
        for (const auto& entry : defaults.toArray()) {
            auto recommendation = object(entry, "arguments.default-user-jvm[]");
            keys(recommendation, {"rules", "value"}, "arguments.default-user-jvm[]");
            if (recommendation.contains("rules")) {
                if (!recommendation.value("rules").isArray()) { invalid("rules"); }
                QJsonArray normalized;
                for (const auto& item : recommendation.value("rules").toArray()) {
                    auto rule = object(item, "rules[]");
                    if (rule.contains("os")) {
                        auto os = object(rule.value("os"), "rules.os");
                        if (os.contains("versionRange")) {
                            const auto range = object(os.value("versionRange"), "rules.os.versionRange");
                            keys(range, {"min", "max"}, "rules.os.versionRange");
                            if (range.isEmpty()) { invalid("rules.os.versionRange"); }
                            for (auto it = range.begin(); it != range.end(); ++it) {
                                const auto parts = text(it.value(), "rules.os.versionRange").split('.');
                                if (parts.size() != 3) { invalid("rules.os.versionRange"); }
                                for (const auto& part : parts) {
                                    if (part.isEmpty()) { invalid("rules.os.versionRange"); }
                                    for (const auto ch : part) {
                                        if (ch < '0' || ch > '9') { invalid("rules.os.versionRange"); }
                                    }
                                }
                            }
                            os.remove("versionRange");
                        }
                        rule.insert("os", os);
                    }
                    normalized.append(rule);
                }
                recommendation.insert("rules", normalized);
            } else {
                recommendation.insert("rules", QJsonArray{QJsonObject{{"action", "allow"}}});
            }
            (void)arguments(QJsonArray{recommendation});
        }
    }
    result.jvmArguments = arguments(args.value("jvm"));
    result.gameArguments = arguments(args.value("game"));
    return result;
}

} // namespace

MetadataResult parseVersionMetadata(const QByteArray& json)
{
    if (json.size() > 4 * 1024 * 1024) { return core::PlanError{core::PlanErrorCode::InvalidMetadata, "json.size"}; }
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return core::PlanError{core::PlanErrorCode::InvalidMetadata, "json"};
    }
    try {
        return parse(document.object());
    } catch (const ParseFailure& failure) {
        return failure.error;
    }
}

} // namespace hustmcl::services
