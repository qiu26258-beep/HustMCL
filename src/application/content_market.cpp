#include <hustmcl/application/content_market.h>
#include <QPointer>
#include <QUuid>
#include <algorithm>

namespace hustmcl::application {
ContentController::ContentController(ContentSource& source, ContentDownloadStore& files, QObject* parent)
    : QObject(parent), source_(source), files_(files) {}
ContentController::~ContentController() {
    const auto id = state_.taskId;
    state_.taskId.clear();
    source_.cancel(id); files_.cancel(id);
}
bool ContentController::fileBusy() const { return state_.phase == ContentPhase::Downloading || state_.phase == ContentPhase::Cancelling; }
QString ContentController::begin(ContentPhase phase) {
    const auto old = state_.taskId;
    state_.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    source_.cancel(old);
    state_.phase = phase; state_.error = ContentError::None; state_.downloadPath.clear(); state_.reused = false;
    return state_.taskId;
}
void ContentController::fail(ContentError error) {
    state_.error = error; state_.phase = error == ContentError::Cancelled ? ContentPhase::Cancelled : ContentPhase::Failed;
    emit changed(state_);
}
bool ContentController::search(const core::ContentQuery& q) {
    if (fileBusy()) return false;
    if (!core::validContentQuery(q)) { begin(ContentPhase::Failed); fail(ContentError::InvalidRequest); return false; }
    const auto id = begin(ContentPhase::Searching);
    state_.query = q; state_.page = {}; state_.selectedProject.reset(); state_.versions.clear(); emit changed(state_);
    const QPointer<ContentController> guard(this);
    source_.search(id, q, [guard, id](ContentResult<core::ContentPage> result) {
        if (!guard || guard->state_.taskId != id || (guard->state_.phase != ContentPhase::Searching && guard->state_.phase != ContentPhase::Cancelling)) return;
        if (guard->state_.phase == ContentPhase::Cancelling) { guard->fail(ContentError::Cancelled); return; }
        if (const auto* e = std::get_if<ContentError>(&result)) { guard->fail(*e); return; }
        guard->state_.page = std::get<core::ContentPage>(std::move(result)); guard->state_.phase = ContentPhase::Ready; emit guard->changed(guard->state_);
    });
    return true;
}
bool ContentController::selectProject(const QString& projectId) {
    if (fileBusy()) return false;
    const auto found = std::find_if(state_.page.projects.begin(), state_.page.projects.end(), [&](const auto& p) { return p.id == projectId.toStdString(); });
    if (found == state_.page.projects.end()) return false;
    const auto project = *found;
    const auto id = begin(ContentPhase::LoadingVersions);
    state_.selectedProject = project; state_.versions.clear(); emit changed(state_);
    const QPointer<ContentController> guard(this);
    source_.versions(id, project.id, state_.query, [guard, id](ContentResult<std::vector<core::ContentVersion>> result) {
        if (!guard || guard->state_.taskId != id || (guard->state_.phase != ContentPhase::LoadingVersions && guard->state_.phase != ContentPhase::Cancelling)) return;
        if (guard->state_.phase == ContentPhase::Cancelling) { guard->fail(ContentError::Cancelled); return; }
        if (const auto* e = std::get_if<ContentError>(&result)) { guard->fail(*e); return; }
        auto versions = std::get<std::vector<core::ContentVersion>>(std::move(result));
        std::erase_if(versions, [&](const auto& v) { return v.projectId != guard->state_.selectedProject->id || !core::contentDownloadFile(v, guard->state_.query); });
        guard->state_.versions = std::move(versions); guard->state_.phase = ContentPhase::Ready; emit guard->changed(guard->state_);
    });
    return true;
}
bool ContentController::download(const QString& versionId) {
    if (fileBusy() || state_.phase == ContentPhase::Searching || state_.phase == ContentPhase::LoadingVersions || !state_.selectedProject) return false;
    const auto found = std::find_if(state_.versions.begin(), state_.versions.end(), [&](const auto& v) { return v.id == versionId.toStdString(); });
    if (found == state_.versions.end()) return false;
    const auto file = core::contentDownloadFile(*found, state_.query);
    if (!file) return false;
    const ContentDownloadRequest request{state_.selectedProject->id, found->id, *file};
    const auto id = begin(ContentPhase::Downloading); emit changed(state_);
    const QPointer<ContentController> guard(this);
    files_.acquire(id, request, [guard, id](ContentResult<ContentDownloadResult> result) {
        if (!guard || guard->state_.taskId != id || !guard->fileBusy()) return;
        if (const auto* e = std::get_if<ContentError>(&result)) { guard->fail(*e); return; }
        // Atomic publication can win a racing cancellation; never report it as rolled back.
        const auto value = std::get<ContentDownloadResult>(std::move(result));
        guard->state_.downloadPath = value.path; guard->state_.reused = value.reused; guard->state_.phase = ContentPhase::Downloaded; emit guard->changed(guard->state_);
    });
    return true;
}
bool ContentController::cancel(const QString& id) {
    if (id != state_.taskId || (state_.phase != ContentPhase::Searching && state_.phase != ContentPhase::LoadingVersions && state_.phase != ContentPhase::Downloading)) return false;
    const bool downloading = state_.phase == ContentPhase::Downloading;
    state_.phase = ContentPhase::Cancelling; emit changed(state_);
    if (downloading) files_.cancel(id); else source_.cancel(id);
    return true;
}
} // namespace hustmcl::application
