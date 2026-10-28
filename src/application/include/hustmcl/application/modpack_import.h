#pragma once
#include <hustmcl/core/modpack.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <optional>
#include <variant>

namespace hustmcl::application {
enum class ImportError { InvalidArchive, UnsupportedFormat, UnsafePath, InvalidManifest, UnsupportedLoader,
    Network, Integrity, Storage, Conflict, JavaUnavailable, Cancelled, Busy };
struct ImportFailure { ImportError code = ImportError::InvalidArchive; QString context; };
template<class T> using ImportResult = std::variant<T, ImportFailure>;
struct InstalledInstance {
    QString id, name, versionLabel, root, versionId, javaPath;
};
struct ImportProgress {
    std::uint64_t completedFiles = 0, totalFiles = 0, completedBytes = 0, totalBytes = 0;
};
// One private workspace per task. All callbacks are delivered on the owning event thread.
// Each phase settles once; cancel waits for IO to stop. release never removes user data.
class ModpackWorkspace {
public:
    using Progress = std::function<void(ImportProgress)>;
    using Done = std::function<void(ImportResult<std::monostate>)>;
    virtual ~ModpackWorkspace() = default;
    virtual void inspect(const QString& task, const QString& archive, std::function<void(ImportResult<core::Modpack>)>) = 0;
    virtual void inspectVanilla(const QString&, const QString&, std::function<void(ImportResult<core::Modpack>)> done) { done(ImportFailure{ImportError::UnsupportedFormat, {}}); }
    virtual void listVanillaVersions(std::function<void(ImportResult<QStringList>)> done) { done(QStringList{}); }
    virtual void prepareGame(const QString& task, Progress, Done) = 0;
    virtual void installFiles(const QString& task, Progress, Done) = 0;
    virtual void commit(const QString& task, std::function<void(ImportResult<InstalledInstance>)>) = 0;
    virtual void cancel(const QString& task) = 0;
    virtual void release(const QString& task) = 0;
    virtual void listInstalled(std::function<void(ImportResult<std::vector<InstalledInstance>>)>) = 0;
};
enum class ImportPhase { Idle, Inspecting, PreparingGame, InstallingFiles, Committing, Succeeded, Cancelling, Cancelled, Failed };
struct ModpackSnapshot {
    QString taskId, archivePath, packName, selectedId;
    ImportPhase phase = ImportPhase::Idle;
    ImportProgress progress;
    std::optional<ImportFailure> error;
    std::vector<InstalledInstance> instances;
    bool launchActive = false;
    QString vanillaVersion;
    QStringList vanillaVersions;
    bool versionsLoading = false;
    std::optional<ImportFailure> versionsError;
};
[[nodiscard]] bool importActive(ImportPhase phase);
class ModpackController final : public QObject {
    Q_OBJECT
public:
    ModpackController(ModpackWorkspace&, InstalledInstance bundled, QObject* parent = nullptr);
    ~ModpackController() override;
    ModpackSnapshot snapshot() const { return state_; }
    bool importFile(const QString& localPath);
    bool installVanilla(const QString& version);
    void loadVanillaVersions();
    bool cancel(const QString& taskId);
    bool retry(const QString& taskId);
    bool selectInstance(const QString& instanceId);
    void setLaunchActive(bool active);
signals:
    void changed(const hustmcl::application::ModpackSnapshot& snapshot);
    void instanceSelected(const hustmcl::application::InstalledInstance& instance);
private:
    void publish();
    bool begin(const QString& localPath, const QString& vanillaVersion);
    bool current(const QString&, ImportPhase) const;
    void progress(const QString&, ImportPhase, ImportProgress);
    void failed(const QString&, ImportFailure);
    void prepared(const QString&, ImportResult<std::monostate>);
    void filesInstalled(const QString&, ImportResult<std::monostate>);
    ModpackWorkspace& workspace_;
    ModpackSnapshot state_;
};
}
Q_DECLARE_METATYPE(hustmcl::application::ModpackSnapshot)
Q_DECLARE_METATYPE(hustmcl::application::InstalledInstance)
