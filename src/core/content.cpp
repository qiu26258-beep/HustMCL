#include <hustmcl/core/content.h>
#include <hustmcl/core/detail/download_validation.h>
#include <algorithm>

namespace hustmcl::core {
namespace {
bool asciiId(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }
bool filter(const std::string& s) {
    return s.size() <= 64 && std::all_of(s.begin(), s.end(), [](char c) { return asciiId(c) || c == '.' || c == '-' || c == '_'; });
}
}
bool validContentId(const std::string& id) { return id.size() == 8 && std::all_of(id.begin(), id.end(), asciiId); }
bool validContentQuery(const ContentQuery& q) {
    return q.text.size() <= 512 && std::none_of(q.text.begin(), q.text.end(), [](unsigned char c) { return c < 32; })
        && filter(q.gameVersion) && (q.loader.empty() || q.loader == "fabric" || q.loader == "forge" || q.loader == "neoforge" || q.loader == "quilt")
        && q.offset >= 0 && q.offset <= 10000 && q.limit >= 1 && q.limit <= 100;
}
bool validContentFile(const ContentFile& f) {
    const auto& n = f.name;
    if (n.empty() || n.size() > 180 || n.back() == '.' || n.back() == ' ' || n.size() < 5 || n.substr(n.size()-4) != ".jar") return false;
    if (std::any_of(n.begin(), n.end(), [](unsigned char c) { return c < 32 || std::string("\\/:*?\"<>|").find(static_cast<char>(c)) != std::string::npos; })) return false;
    auto stem = n.substr(0, n.find('.'));
    std::transform(stem.begin(), stem.end(), stem.begin(), [](char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c-'a'+'A') : c; });
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
        (stem.size() == 4 && (stem.substr(0,3) == "COM" || stem.substr(0,3) == "LPT") && stem[3] >= '1' && stem[3] <= '9')) return false;
    const auto& d = f.download;
    return d.size > 0 && d.size <= 512ULL * 1024 * 1024 && detail::validSha1(d.sha1)
        && d.url.starts_with("https://cdn.modrinth.com/") && d.url.size() <= 4096
        && std::none_of(d.url.begin(), d.url.end(), [](unsigned char c) { return c <= 32 || c == '\\' || c == '#'; });
}
bool contentCompatible(const ContentVersion& v, const ContentQuery& q) {
    return validContentQuery(q) && (q.gameVersion.empty() || std::find(v.gameVersions.begin(), v.gameVersions.end(), q.gameVersion) != v.gameVersions.end())
        && (q.loader.empty() || std::find(v.loaders.begin(), v.loaders.end(), q.loader) != v.loaders.end());
}
std::optional<ContentFile> contentDownloadFile(const ContentVersion& v, const ContentQuery& q) {
    if (!validContentId(v.id) || !validContentId(v.projectId) || !contentCompatible(v, q)) return {};
    const ContentFile* selected = nullptr;
    for (const auto& f : v.files) if (f.primary) { if (selected) return {}; selected = &f; }
    if (!selected && v.files.size() == 1) selected = &v.files.front();
    if (!selected || !validContentFile(*selected)) return {};
    return *selected;
}
} // namespace hustmcl::core
