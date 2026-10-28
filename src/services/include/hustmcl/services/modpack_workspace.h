#pragma once
#include <hustmcl/application/modpack_import.h>
#include <QObject>
#include <memory>

namespace hustmcl::services {
class QtModpackWorkspace final : public QObject, public application::ModpackWorkspace {
public:
    struct Options {
        QString versionManifestUrl = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json";
        QString fabricMetaBase = "https://meta.fabricmc.net/v2/versions/loader/";
        QString javaApiBase = "https://api.adoptium.net/v3/assets/latest/";
        bool allowLoopbackHttp = false;
        int timeoutMs = 30000;
    };
    QtModpackWorkspace(QString root,QString bundledJava,Options options,QObject* parent=nullptr);
    ~QtModpackWorkspace() override;
    void inspect(const QString&,const QString&,std::function<void(application::ImportResult<core::Modpack>)>) override;
    void inspectVanilla(const QString&,const QString&,std::function<void(application::ImportResult<core::Modpack>)>) override;
    void listVanillaVersions(std::function<void(application::ImportResult<QStringList>)>) override;
    void prepareGame(const QString&,Progress,Done) override;
    void installFiles(const QString&,Progress,Done) override;
    void commit(const QString&,std::function<void(application::ImportResult<application::InstalledInstance>)>) override;
    void cancel(const QString&) override;
    void release(const QString&) override;
    void listInstalled(std::function<void(application::ImportResult<std::vector<application::InstalledInstance>>)>) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Read-only checks for imported payloads, used again immediately before launch.
bool verifyImportedPackFiles(const QString& root, bool requireRecord = false);
}
