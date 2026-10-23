#pragma once
#include <hustmcl/core/launch_plan.h>

namespace hustmcl::core {
enum class ContentProvider { Modrinth };
struct ContentQuery {
    std::string text, gameVersion, loader;
    int offset = 0;
    int limit = 20;
};
struct ContentProject {
    ContentProvider provider = ContentProvider::Modrinth;
    std::string id, slug, title, description;
    std::uint64_t downloads = 0;
};
struct ContentPage {
    std::vector<ContentProject> projects;
    int offset = 0, limit = 20, total = 0;
};
struct ContentFile {
    std::string name;
    DownloadDescriptor download;
    bool primary = false;
};
struct ContentDependency {
    std::string projectId, versionId, type;
};
struct ContentVersion {
    std::string id, projectId, name, number, releaseType;
    std::vector<std::string> gameVersions, loaders;
    std::vector<ContentFile> files;
    std::vector<ContentDependency> dependencies;
};
bool validContentId(const std::string& id);
bool validContentQuery(const ContentQuery& query);
bool validContentFile(const ContentFile& file);
bool contentCompatible(const ContentVersion& version, const ContentQuery& query);
// Select an explicit primary file, or the sole file. Ambiguous files are rejected.
std::optional<ContentFile> contentDownloadFile(const ContentVersion& version, const ContentQuery& query);
} // namespace hustmcl::core
