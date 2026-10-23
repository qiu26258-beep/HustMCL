#pragma once

#include <hustmcl/core/java_runtime.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace hustmcl::core {

enum class RuleAction { Allow, Disallow };
enum class OperatingSystem { Windows, Linux, MacOS };

struct VersionRule {
    RuleAction action = RuleAction::Allow;
    std::optional<OperatingSystem> os;
    std::optional<RuntimeArchitecture> architecture;
    std::map<std::string, bool> features;
};

// An absent list is unconditional; an explicit empty list allows nothing.
using VersionRules = std::optional<std::vector<VersionRule>>;

struct DownloadDescriptor {
    std::string url;
    std::string sha1;
    std::uint64_t size = 0;
};

struct VersionLibrary {
    std::string name;
    std::filesystem::path relativePath;
    DownloadDescriptor download;
    VersionRules rules;
};

struct VersionArgument {
    std::vector<std::string> values;
    VersionRules rules;
};

// Normalized input. No JSON, file access, network or account credentials in core.
struct VersionMetadata {
    std::string id;
    std::string type;
    std::string mainClass;
    JavaRequirement java{0};
    DownloadDescriptor client;
    std::string assetIndexId;
    DownloadDescriptor assetIndex;
    std::string loggingId;
    DownloadDescriptor loggingFile;
    std::string loggingArgument;
    std::vector<VersionLibrary> libraries;
    std::vector<VersionArgument> jvmArguments;
    std::vector<VersionArgument> gameArguments;
};

enum class PlanErrorCode {
    InvalidMetadata,
    UnsupportedMetadata,
    InvalidPaths,
    InvalidMemory,
    NoCompatibleJava,
    InvalidManualJava,
    FilePathCollision,
    UnknownPlaceholder,
    SizeOverflow,
};

struct PlanError {
    PlanErrorCode code;
    // Schema location only; never include metadata values or credentials here.
    std::string field;
};

struct Resolution { int width; int height; };

struct LaunchContext {
    std::filesystem::path dataRoot;
    std::filesystem::path gameDirectory;
    std::filesystem::path nativesDirectory;
    std::vector<JavaRuntime> runtimes;
    std::optional<JavaRuntime> manualRuntime;
    std::uint32_t maximumMemoryMiB = 2048;
    std::optional<Resolution> resolution;
    std::string launcherVersion = "0.1.0";
};

enum class SessionField { PlayerName, Uuid, AccessToken, ClientId, Xuid };
// UTF-8 literal parts and typed session references; paths are never reparsed as templates.
using ArgumentPart = std::variant<std::string, SessionField>;
using LaunchArgument = std::vector<ArgumentPart>;

enum class FileRole { Library, Client, AssetIndex, LoggingConfig, AssetObject };
struct PlannedFile {
    std::filesystem::path relativePath; // relative to dataRoot
    DownloadDescriptor download;
    FileRole role;
};

struct LaunchPlan {
    JavaRuntime runtime;
    std::filesystem::path workingDirectory;
    std::vector<std::filesystem::path> classpath;
    std::vector<LaunchArgument> arguments; // JVM flags, main class, then game flags
    std::vector<PlannedFile> files;
    // Base requirements only. buildInstallFilePlan expands the referenced asset index.
};

using RuleDecision = std::variant<bool, PlanError>;
[[nodiscard]] RuleDecision evaluateWindowsX64Rules(
    const VersionRules& rules, const std::map<std::string, bool>& features = {});

using VersionFilesResult = std::variant<std::vector<PlannedFile>, PlanError>;
// Shared Windows x64 file selection; no installed Java or session is required.
[[nodiscard]] VersionFilesResult buildVersionFilePlan(
    const VersionMetadata& version, const std::map<std::string, bool>& features = {});

using LaunchPlanResult = std::variant<LaunchPlan, PlanError>;
[[nodiscard]] LaunchPlanResult buildLaunchPlan(const VersionMetadata& version, const LaunchContext& context);

} // namespace hustmcl::core
