#include <hustmcl/core/launch_plan.h>
#include <hustmcl/core/detail/download_validation.h>

#include <algorithm>
#include <array>
#include <set>
#include <string_view>
#include <utility>

namespace hustmcl::core {
namespace {

constexpr std::array<std::string_view, 6> knownFeatures{
    "is_demo_user", "has_custom_resolution", "has_quick_plays_support",
    "is_quick_play_singleplayer", "is_quick_play_multiplayer", "is_quick_play_realms"};

bool safeText(std::string_view value)
{
    return !value.empty() && std::none_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch < 32 || ch == 127;
    });
}

bool reservedComponent(std::string_view value)
{
    std::string stem(value.substr(0, value.find('.')));
    std::transform(stem.begin(), stem.end(), stem.begin(), [](char ch) {
        return ch >= 'a' && ch <= 'z' ? static_cast<char>(ch - 'a' + 'A') : ch;
    });
    return stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL"
        || stem == "CONIN$" || stem == "CONOUT$"
        || (stem.size() == 4 && (stem.starts_with("COM") || stem.starts_with("LPT"))
            && stem[3] >= '1' && stem[3] <= '9');
}

bool safeComponent(std::string_view value, bool allowMavenPlus = false)
{
    return safeText(value) && value != "." && value.back() != '.' && !reservedComponent(value)
        && std::all_of(value.begin(), value.end(), [allowMavenPlus](char ch) {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '_'
                || (allowMavenPlus && ch == '+');
        });
}

bool validMainClass(std::string_view value)
{
    bool start = true;
    for (const char ch : value) {
        if (ch == '.') {
            if (start) { return false; }
            start = true;
        } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '$'
                   || (!start && ch >= '0' && ch <= '9')) {
            start = false;
        } else { return false; }
    }
    return !start;
}

std::string utf8(const std::filesystem::path& path)
{
    auto preferred = path;
    preferred.make_preferred();
    const auto value = preferred.u8string();
    return {value.begin(), value.end()};
}

bool safeRelative(const std::filesystem::path& path)
{
    if (path.empty() || path.has_root_path()) { return false; }
    return std::all_of(path.begin(), path.end(), [](const auto& part) {
        return safeComponent(utf8(part), true);
    });
}

bool safeAbsolute(const std::filesystem::path& path)
{
    if (!path.is_absolute()) { return false; }
    const auto root = path.root_name().wstring();
    if (root.size() != 2 || root[1] != L':' || !((root[0] >= L'A' && root[0] <= L'Z')
        || (root[0] >= L'a' && root[0] <= L'z'))) { return false; }
    const auto text = path.relative_path().wstring();
    if (std::any_of(text.begin(), text.end(), [](wchar_t ch) {
        return ch < 32 || std::wstring_view(L";:\"<>|?*").find(ch) != std::wstring_view::npos;
    })) { return false; }
    for (const auto& part : path.relative_path()) {
        const auto name = part.wstring();
        if (name.empty() || name == L"." || name == L".." || name.back() == L'.' || name.back() == L' '
            || reservedComponent(utf8(part))) {
            return false;
        }
    }
    return true;
}

bool validDownload(const DownloadDescriptor& file)
{
    return safeText(file.url) && file.url.starts_with("https://") && file.url.size() > 8
        && file.size > 0 && detail::validSha1(file.sha1);
}

using ArgumentResult = std::variant<LaunchArgument, PlanError>;
ArgumentResult expandArgument(std::string_view text, const std::map<std::string, std::string>& variables)
{
    static const std::map<std::string, SessionField> sessions{
        {"auth_player_name", SessionField::PlayerName}, {"auth_uuid", SessionField::Uuid},
        {"auth_access_token", SessionField::AccessToken}, {"clientid", SessionField::ClientId},
        {"auth_xuid", SessionField::Xuid}};
    if (!safeText(text)) { return PlanError{PlanErrorCode::InvalidMetadata, "arguments.value"}; }
    LaunchArgument result;
    std::size_t position = 0;
    while (position < text.size()) {
        const auto start = text.find("${", position);
        if (start == std::string_view::npos) {
            result.emplace_back(std::string(text.substr(position)));
            break;
        }
        if (start > position) { result.emplace_back(std::string(text.substr(position, start - position))); }
        const auto end = text.find('}', start + 2);
        if (end == std::string_view::npos) {
            return PlanError{PlanErrorCode::UnknownPlaceholder, "arguments.placeholder"};
        }
        const std::string key(text.substr(start + 2, end - start - 2));
        if (const auto it = variables.find(key); it != variables.end()) {
            result.emplace_back(it->second);
        } else if (const auto session = sessions.find(key); session != sessions.end()) {
            result.emplace_back(session->second);
        } else {
            return PlanError{PlanErrorCode::UnknownPlaceholder, "arguments.placeholder"};
        }
        position = end + 1;
    }
    return result;
}

} // namespace

RuleDecision evaluateWindowsX64Rules(const VersionRules& rules, const std::map<std::string, bool>& features)
{
    bool allowed = !rules.has_value();
    if (!rules) { return allowed; }
    for (const auto& rule : *rules) {
        bool matches = (!rule.os || *rule.os == OperatingSystem::Windows)
            && (!rule.architecture || *rule.architecture == RuntimeArchitecture::X64);
        for (const auto& [name, expected] : rule.features) {
            if (std::find(knownFeatures.begin(), knownFeatures.end(), name) == knownFeatures.end()) {
                return PlanError{PlanErrorCode::UnsupportedMetadata, "rules.features"};
            }
            const auto value = features.find(name);
            const bool actual = value != features.end() && value->second;
            matches = matches && actual == expected;
        }
        if (matches) { allowed = rule.action == RuleAction::Allow; }
    }
    return allowed;
}

VersionFilesResult buildVersionFilePlan(const VersionMetadata& version, const std::map<std::string, bool>& features)
{
    if (!safeComponent(version.id) || version.type != "release" || !safeComponent(version.assetIndexId)
        || !safeComponent(version.loggingId) || version.java.majorVersion <= 0) {
        return PlanError{PlanErrorCode::InvalidMetadata, "version"};
    }
    std::vector<PlannedFile> files;
    std::set<std::string> targets;
    const auto addFile = [&](std::filesystem::path relative, const DownloadDescriptor& source,
                             FileRole role) -> std::optional<PlanError> {
        if (!safeRelative(relative) || !validDownload(source)) {
            return PlanError{PlanErrorCode::InvalidMetadata, "downloads"};
        }
        auto key = relative.generic_string();
        std::transform(key.begin(), key.end(), key.begin(), [](char ch) {
            return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
        });
        for (auto slash = key.find('/'); slash != std::string::npos; slash = key.find('/', slash + 1)) {
            if (targets.contains(key.substr(0, slash))) { return PlanError{PlanErrorCode::FilePathCollision, "downloads.path"}; }
        }
        const auto descendant = targets.lower_bound(key + '/');
        if ((descendant != targets.end() && descendant->starts_with(key + '/')) || !targets.insert(key).second) {
            return PlanError{PlanErrorCode::FilePathCollision, "downloads.path"};
        }
        files.push_back({std::move(relative), source, role});
        return std::nullopt;
    };

    for (const auto& library : version.libraries) {
        const auto decision = evaluateWindowsX64Rules(library.rules, features);
        if (const auto error = std::get_if<PlanError>(&decision)) { return *error; }
        if (!std::get<bool>(decision)) { continue; }
        if (auto error = addFile(std::filesystem::path("libraries") / library.relativePath,
                                 library.download, FileRole::Library)) { return *error; }
    }
    const auto clientPath = std::filesystem::path("versions") / version.id / (version.id + ".jar");
    if (auto error = addFile(clientPath, version.client, FileRole::Client)) { return *error; }
    if (auto error = addFile(std::filesystem::path("assets/indexes") / (version.assetIndexId + ".json"),
                             version.assetIndex, FileRole::AssetIndex)) { return *error; }
    const auto loggingPath = std::filesystem::path("assets/log_configs") / version.loggingId;
    if (auto error = addFile(loggingPath, version.loggingFile, FileRole::LoggingConfig)) { return *error; }

    return files;
}

LaunchPlanResult buildLaunchPlan(const VersionMetadata& version, const LaunchContext& context)
{
    const std::map<std::string, bool> features{{"has_custom_resolution", context.resolution.has_value()}};
    auto files = buildVersionFilePlan(version, features);
    if (const auto error = std::get_if<PlanError>(&files)) { return *error; }
    if (!validMainClass(version.mainClass) || version.jvmArguments.empty() || version.gameArguments.empty()) {
        return PlanError{PlanErrorCode::InvalidMetadata, "version"};
    }
    if (!safeAbsolute(context.dataRoot) || !safeAbsolute(context.gameDirectory)
        || !safeAbsolute(context.nativesDirectory) || !safeText(context.launcherVersion)) {
        return PlanError{PlanErrorCode::InvalidPaths, "context.paths"};
    }
    if (context.maximumMemoryMiB < 256 || context.maximumMemoryMiB > 65536) {
        return PlanError{PlanErrorCode::InvalidMemory, "context.maximumMemoryMiB"};
    }
    if (context.resolution && (context.resolution->width <= 0 || context.resolution->height <= 0)) {
        return PlanError{PlanErrorCode::InvalidMetadata, "context.resolution"};
    }
    const auto selection = selectJavaRuntime(version.java, context.runtimes, context.manualRuntime);
    if (const auto error = std::get_if<JavaSelectionError>(&selection)) {
        return PlanError{*error == JavaSelectionError::InvalidManualRuntime
            ? PlanErrorCode::InvalidManualJava : PlanErrorCode::NoCompatibleJava, "context.java"};
    }
    const auto runtime = std::get<JavaRuntime>(selection);
    if (!safeAbsolute(runtime.executable)) { return PlanError{PlanErrorCode::InvalidPaths, "context.java"}; }

    LaunchPlan plan{runtime, context.gameDirectory, {}, {}, {}};
    plan.files = std::get<std::vector<PlannedFile>>(std::move(files));
    for (const auto& file : plan.files) {
        if (file.role == FileRole::Library || file.role == FileRole::Client) {
            plan.classpath.push_back(context.dataRoot / file.relativePath);
        }
    }
    const auto loggingPath = std::filesystem::path("assets/log_configs") / version.loggingId;

    std::string classpath;
    for (const auto& path : plan.classpath) {
        if (!classpath.empty()) { classpath += ';'; }
        classpath += utf8(path);
    }
    std::map<std::string, std::string> variables{
        {"version_name", version.id}, {"version_type", version.type},
        {"game_directory", utf8(context.gameDirectory)}, {"assets_root", utf8(context.dataRoot / "assets")},
        {"assets_index_name", version.assetIndexId}, {"natives_directory", utf8(context.nativesDirectory)},
        {"launcher_name", "HustMCL"}, {"launcher_version", context.launcherVersion},
        {"classpath", classpath}, {"classpath_separator", ";"}, {"user_type", "msa"}};
    if (context.resolution) {
        variables.emplace("resolution_width", std::to_string(context.resolution->width));
        variables.emplace("resolution_height", std::to_string(context.resolution->height));
    }
    plan.arguments.push_back({std::string("-Xmx") + std::to_string(context.maximumMemoryMiB) + "M"});
    const auto addArguments = [&](const std::vector<VersionArgument>& arguments) -> std::optional<PlanError> {
        for (const auto& argument : arguments) {
            const auto decision = evaluateWindowsX64Rules(argument.rules, features);
            if (const auto error = std::get_if<PlanError>(&decision)) { return *error; }
            if (!std::get<bool>(decision)) { continue; }
            if (argument.values.empty()) { return PlanError{PlanErrorCode::InvalidMetadata, "arguments.value"}; }
            for (const auto& value : argument.values) {
                auto expanded = expandArgument(value, variables);
                if (const auto error = std::get_if<PlanError>(&expanded)) { return *error; }
                plan.arguments.push_back(std::get<LaunchArgument>(std::move(expanded)));
            }
        }
        return std::nullopt;
    };
    if (auto error = addArguments(version.jvmArguments)) { return *error; }
    auto logVariables = variables;
    logVariables.emplace("path", utf8(context.dataRoot / loggingPath));
    auto logging = expandArgument(version.loggingArgument, logVariables);
    if (const auto error = std::get_if<PlanError>(&logging)) { return *error; }
    plan.arguments.push_back(std::get<LaunchArgument>(std::move(logging)));
    plan.arguments.push_back({version.mainClass});
    if (auto error = addArguments(version.gameArguments)) { return *error; }
    return plan;
}

} // namespace hustmcl::core
