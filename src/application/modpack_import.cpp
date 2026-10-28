#include <hustmcl/application/modpack_import.h>
#include <QPointer>
#include <QUuid>
#include <algorithm>

namespace hustmcl::application {
bool importActive(ImportPhase p) {
    return p == ImportPhase::Inspecting || p == ImportPhase::PreparingGame || p == ImportPhase::InstallingFiles
        || p == ImportPhase::Committing || p == ImportPhase::Cancelling;
}
ModpackController::ModpackController(ModpackWorkspace& workspace, InstalledInstance bundled, QObject* parent)
    : QObject(parent), workspace_(workspace) {
    qRegisterMetaType<ModpackSnapshot>(); qRegisterMetaType<InstalledInstance>();
    if (!bundled.id.isEmpty()) { state_.selectedId = bundled.id; state_.instances.push_back(std::move(bundled)); }
    QPointer<ModpackController> guard(this);
    workspace_.listInstalled([guard](auto result) {
        if (!guard) return;
        if (auto list = std::get_if<std::vector<InstalledInstance>>(&result)) {
            for (auto& i : *list) if (std::none_of(guard->state_.instances.begin(), guard->state_.instances.end(), [&](const auto& old){return old.id == i.id;}))
                guard->state_.instances.push_back(std::move(i));
            guard->publish();
        }
    });
}
ModpackController::~ModpackController() { if (importActive(state_.phase)) workspace_.cancel(state_.taskId); }
void ModpackController::publish() { const auto copy = state_; emit changed(copy); }
bool ModpackController::current(const QString& task, ImportPhase phase) const { return state_.taskId == task && state_.phase == phase; }
bool ModpackController::importFile(const QString& path) {
    if (path.isEmpty()) return false;
    return begin(path, {});
}
bool ModpackController::installVanilla(const QString& version) {
    if (!core::validPackVersion(version.toStdString())) return false;
    return begin({}, version);
}
void ModpackController::loadVanillaVersions() {
    if (state_.versionsLoading) return;
    state_.versionsLoading = true; state_.versionsError.reset();
    QPointer<ModpackController> guard(this); publish();
    if (!guard) return;
    workspace_.listVanillaVersions([guard](auto result) {
        if (!guard) return;
        guard->state_.versionsLoading = false;
        if (const auto* versions = std::get_if<QStringList>(&result)) guard->state_.vanillaVersions = *versions;
        else guard->state_.versionsError = std::get<ImportFailure>(result);
        guard->publish();
    });
}
bool ModpackController::begin(const QString& path, const QString& vanillaVersion) {
    if (importActive(state_.phase) || state_.launchActive) return false;
    state_.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    state_.archivePath = path; state_.vanillaVersion = vanillaVersion;
    state_.packName.clear(); state_.error.reset(); state_.progress = {};
    state_.phase = ImportPhase::Inspecting;
    const auto task = state_.taskId;
    QPointer<ModpackController> guard(this);
    publish();
    if (!guard || !current(task, ImportPhase::Inspecting)) return true;
    auto inspected = [guard, task](ImportResult<core::Modpack> result) {
        if (!guard || guard->state_.taskId != task) return;
        if (auto error = std::get_if<ImportFailure>(&result)) { guard->failed(task, *error); return; }
        if (guard->state_.phase == ImportPhase::Cancelling) { guard->failed(task,{ImportError::Cancelled,{}}); return; }
        if (!guard->current(task,ImportPhase::Inspecting)) return;
        const auto& pack = std::get<core::Modpack>(result);
        if (!core::validModpack(pack)) { guard->failed(task,{ImportError::InvalidManifest,{}}); return; }
        guard->state_.packName = QString::fromStdString(pack.name);
        guard->state_.phase = ImportPhase::PreparingGame; guard->publish();
        if (!guard) return;
        guard->workspace_.prepareGame(task, [guard, task](auto p){if(guard)guard->progress(task,ImportPhase::PreparingGame,p);},
            [guard, task](auto r){if(guard)guard->prepared(task,std::move(r));});
    };
    if (vanillaVersion.isEmpty()) workspace_.inspect(task, path, std::move(inspected));
    else workspace_.inspectVanilla(task, vanillaVersion, std::move(inspected));
    return true;
}
void ModpackController::progress(const QString& task, ImportPhase phase, ImportProgress p) {
    if (!current(task,phase)) return;
    state_.progress = p; publish();
}
void ModpackController::failed(const QString& task, ImportFailure error) {
    if (state_.taskId != task || !importActive(state_.phase)) return;
    const bool cancelled = state_.phase == ImportPhase::Cancelling || error.code == ImportError::Cancelled;
    state_.phase = cancelled ? ImportPhase::Cancelled : ImportPhase::Failed;
    state_.error = cancelled ? std::optional<ImportFailure>{} : error;
    workspace_.release(task); publish();
}
void ModpackController::prepared(const QString& task, ImportResult<std::monostate> result) {
    if (state_.taskId != task) return;
    if (auto error = std::get_if<ImportFailure>(&result)) {failed(task,*error);return;}
    if (state_.phase == ImportPhase::Cancelling) {failed(task,{ImportError::Cancelled,{}});return;}
    if (!current(task,ImportPhase::PreparingGame)) return;
    state_.phase = ImportPhase::InstallingFiles; state_.progress = {}; QPointer<ModpackController> guard(this); publish();
    if (guard) workspace_.installFiles(task,[guard, task](auto p){if(guard)guard->progress(task,ImportPhase::InstallingFiles,p);},
        [guard, task](auto r){if(guard)guard->filesInstalled(task,std::move(r));});
}
void ModpackController::filesInstalled(const QString& task, ImportResult<std::monostate> result) {
    if (state_.taskId != task) return;
    if (auto error = std::get_if<ImportFailure>(&result)) {failed(task,*error);return;}
    if (state_.phase == ImportPhase::Cancelling) {failed(task,{ImportError::Cancelled,{}});return;}
    if (!current(task,ImportPhase::InstallingFiles)) return;
    state_.phase = ImportPhase::Committing; QPointer<ModpackController> guard(this); publish();
    if (!guard) return;
    workspace_.commit(task,[guard,task](auto r) {
        if (!guard || !guard->current(task,ImportPhase::Committing)) return;
        if (auto error=std::get_if<ImportFailure>(&r)) {guard->failed(task,*error);return;}
        auto instance=std::get<InstalledInstance>(std::move(r));
        guard->state_.instances.push_back(instance); guard->state_.selectedId=instance.id;
        guard->state_.phase=ImportPhase::Succeeded; guard->workspace_.release(task); guard->publish();
        if (guard) emit guard->instanceSelected(instance);
    });
}
bool ModpackController::cancel(const QString& task) {
    if (state_.taskId!=task || !importActive(state_.phase) || state_.phase==ImportPhase::Committing || state_.phase==ImportPhase::Cancelling) return false;
    state_.phase=ImportPhase::Cancelling; QPointer<ModpackController> guard(this); publish();
    if(guard)workspace_.cancel(task);
    return true;
}
bool ModpackController::retry(const QString& task) {
    if(state_.taskId!=task || (state_.phase!=ImportPhase::Failed && state_.phase!=ImportPhase::Cancelled))return false;
    return state_.vanillaVersion.isEmpty() ? importFile(state_.archivePath) : installVanilla(state_.vanillaVersion);
}
bool ModpackController::selectInstance(const QString& id) {
    if(importActive(state_.phase)||state_.launchActive)return false;
    auto it=std::find_if(state_.instances.begin(),state_.instances.end(),[&](const auto& i){return i.id==id;});
    if(it==state_.instances.end())return false;
    auto selected=*it; state_.selectedId=id; QPointer<ModpackController> guard(this); publish();
    if(guard)emit instanceSelected(selected);
    return true;
}
void ModpackController::setLaunchActive(bool active) {if(state_.launchActive!=active){state_.launchActive=active;publish();}}
}
