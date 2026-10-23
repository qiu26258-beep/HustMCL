#include <hustmcl/core/install_plan.h>
#include <hustmcl/core/detail/download_validation.h>

#include <algorithm>
#include <limits>
#include <set>
#include <string_view>
#include <utility>

namespace hustmcl::core {
namespace {
bool validName(std::string_view name)
{
    if (name.empty() || name.size() > 1024 || name.front() == '/' || name.back() == '/') { return false; }
    std::size_t start = 0;
    while (start < name.size()) {
        const auto end = name.find('/', start);
        const auto part = name.substr(start, end == std::string_view::npos ? end : end - start);
        if (part.empty() || part == "." || part == ".." || !std::all_of(part.begin(), part.end(), [](char ch) {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-';
        })) { return false; }
        if (end == std::string_view::npos) { break; }
        start = end + 1;
    }
    return true;
}
} // namespace

std::optional<PlanError> validateAssetIndex(const AssetIndex& index)
{
    if (!detail::validSha1(index.sourceSha1) || index.sourceSize == 0 || index.sourceSize > MaximumAssetIndexBytes
        || index.objects.size() > MaximumAssetObjects) {
        return PlanError{PlanErrorCode::InvalidMetadata, "assetIndex.source"};
    }
    std::set<std::string> names;
    std::map<std::string, std::uint64_t> sizes;
    for (const auto& object : index.objects) {
        if (!validName(object.logicalName) || !names.insert(object.logicalName).second || !detail::validSha1(object.sha1)) {
            return PlanError{PlanErrorCode::InvalidMetadata, "assetIndex.objects"};
        }
        const auto [it, inserted] = sizes.emplace(object.sha1, object.size);
        if (!inserted && it->second != object.size) {
            return PlanError{PlanErrorCode::InvalidMetadata, "assetIndex.objects.sizeConflict"};
        }
    }
    return {};
}

InstallFilePlanResult buildInstallFilePlan(const VersionMetadata& version, const AssetIndex& index,
                                         const std::map<std::string, bool>& features)
{
    auto base = buildVersionFilePlan(version, features);
    if (const auto error = std::get_if<PlanError>(&base)) { return *error; }
    if (auto error = validateAssetIndex(index)) { return *error; }
    if (index.sourceSha1 != version.assetIndex.sha1 || index.sourceSize != version.assetIndex.size) {
        return PlanError{PlanErrorCode::InvalidMetadata, "assetIndex.binding"};
    }
    InstallFilePlan plan{version.id, version.java, std::get<std::vector<PlannedFile>>(std::move(base)), 0, index.objects.size(), 0};
    std::map<std::string, std::uint64_t> unique;
    for (const auto& object : index.objects) { unique.emplace(object.sha1, object.size); }
    plan.uniqueAssetCount = unique.size();
    for (const auto& [hash, size] : unique) {
        const auto suffix = hash.substr(0, 2) + '/' + hash;
        plan.files.push_back({std::filesystem::path("assets/objects") / suffix,
                              {"https://resources.download.minecraft.net/" + suffix, hash, size}, FileRole::AssetObject});
    }
    for (const auto& file : plan.files) {
        if (file.download.size > std::numeric_limits<std::uint64_t>::max() - plan.totalBytes) {
            return PlanError{PlanErrorCode::SizeOverflow, "files.totalBytes"};
        }
        plan.totalBytes += file.download.size;
    }
    return plan;
}

} // namespace hustmcl::core
