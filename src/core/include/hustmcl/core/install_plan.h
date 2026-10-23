#pragma once

#include <hustmcl/core/launch_plan.h>

#include <cstddef>

namespace hustmcl::core {

inline constexpr std::size_t MaximumAssetIndexBytes = 8 * 1024 * 1024;
inline constexpr std::size_t MaximumAssetObjects = 100000;

struct AssetObject {
    std::string logicalName; // Resource identifier, never a physical destination.
    std::string sha1;
    std::uint64_t size = 0;
};

struct AssetIndex {
    // Adapter verifies these against the actual input bytes before normalization.
    std::string sourceSha1;
    std::uint64_t sourceSize = 0;
    std::vector<AssetObject> objects;
};

[[nodiscard]] std::optional<PlanError> validateAssetIndex(const AssetIndex& index);

struct InstallFilePlan {
    std::string versionId;
    JavaRequirement java;
    std::vector<PlannedFile> files; // data-root relative; base files then assets sorted by SHA-1.
    std::uint64_t totalBytes = 0; // Unique destination bytes, before checking existing cache.
    std::size_t logicalAssetCount = 0;
    std::size_t uniqueAssetCount = 0;
};

using InstallFilePlanResult = std::variant<InstallFilePlan, PlanError>;
// Game payload requirements only, not an installation transaction or readiness record.
[[nodiscard]] InstallFilePlanResult buildInstallFilePlan(
    const VersionMetadata& version, const AssetIndex& index,
    const std::map<std::string, bool>& features = {});

} // namespace hustmcl::core
