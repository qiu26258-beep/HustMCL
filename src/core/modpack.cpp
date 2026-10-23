#include <hustmcl/core/modpack.h>
#include <algorithm>
#include <set>

namespace hustmcl::core {
std::string packPathKey(const std::string& value) {
    auto result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : static_cast<char>(c);
    });
    return result;
}
bool validPackPath(const std::string& path) {
    if (path.empty() || path.size() > 220 || path.front() == '/' || path.back() == '/') return false;
    std::size_t start = 0;
    while (start < path.size()) {
        const auto end = path.find('/', start);
        const auto part = path.substr(start, end == std::string::npos ? end : end-start);
        if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ') return false;
        for (const unsigned char c : part) if (c < 32 || c == 127 || std::string("\\:*?\"<>|").find(c) != std::string::npos) return false;
        const auto base = packPathKey(part.substr(0, part.find('.')));
        if (base == "con" || base == "prn" || base == "aux" || base == "nul"
            || (base.size() == 4 && (base.starts_with("com") || base.starts_with("lpt")) && base[3] >= '0' && base[3] <= '9')) return false;
        if (end == std::string::npos) break;
        start = end+1;
    }
    return true;
}
bool packPathsConflict(const std::vector<std::string>& paths) {
    std::set<std::string> names;
    for (const auto& p : paths) {
        if (!validPackPath(p)) return true;
        auto k = packPathKey(p);
        if (!names.insert(k).second) return true;
    }
    for (const auto& k : names) {
        auto next = names.lower_bound(k+'/');
        if (next != names.end() && next->starts_with(k+'/')) return true;
    }
    return false;
}
bool validPackVersion(const std::string& value) {
    return !value.empty() && value.size() <= 80 && value != "." && value != ".."
        && value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-+") == std::string::npos;
}
bool validModpack(const Modpack& pack) {
    if (pack.name.empty() || pack.name.size() > 512 || pack.version.empty() || pack.version.size() > 256
        || !validPackVersion(pack.minecraft) || (pack.loader != PackLoader::Vanilla && !validPackVersion(pack.loaderVersion))
        || pack.files.size() > 50000) return false;
    std::vector<std::string> paths;
    std::uint64_t total = 0;
    for (const auto& f : pack.files) {
        if (f.sha1.size() != 40 || f.sha1.find_first_not_of("0123456789abcdef") != std::string::npos
            || f.sha512.size() != 128 || f.sha512.find_first_not_of("0123456789abcdef") != std::string::npos
            || f.size > 1024ULL*1024*1024 || f.urls.empty() || f.urls.size() > 16) return false;
        total += f.size;
        if (total > 16ULL*1024*1024*1024) return false;
        paths.push_back(f.path);
    }
    return !packPathsConflict(paths);
}
}
