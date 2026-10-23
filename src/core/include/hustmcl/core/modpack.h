#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hustmcl::core {
enum class PackLoader { Vanilla, Fabric, Forge, NeoForge, Quilt };
struct PackFile {
    std::string path, sha1, sha512;
    std::uint64_t size = 0;
    std::vector<std::string> urls;
    bool optional = false;
};
struct Modpack {
    std::string name, version, minecraft, loaderVersion;
    PackLoader loader = PackLoader::Vanilla;
    std::vector<PackFile> files; // Selected client files; server-only entries are excluded by the adapter.
};
// Portable Windows destination rules, also used for archive entries and collision keys.
[[nodiscard]] bool validPackPath(const std::string& path);
[[nodiscard]] std::string packPathKey(const std::string& path);
[[nodiscard]] bool packPathsConflict(const std::vector<std::string>& paths);
[[nodiscard]] bool validPackVersion(const std::string& value);
[[nodiscard]] bool validModpack(const Modpack& pack);
}
