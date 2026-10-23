#include <hustmcl/application/file_acquisition.h>
#include <QPointer>
#include <QUuid>
namespace hustmcl::application {
FileAcquisition::FileAcquisition(FileAcquirer& service, QObject* parent) : QObject(parent), service_(service) {}
FileAcquisition::~FileAcquisition() {
    if (state_.phase == AcquisitionPhase::Acquiring || state_.phase == AcquisitionPhase::Cancelling)
        service_.cancel(state_.taskId);
}
bool FileAcquisition::start(const QString& root, const core::DownloadDescriptor& file) {
    if (state_.phase == AcquisitionPhase::Acquiring || state_.phase == AcquisitionPhase::Cancelling) return false;
    state_ = {QUuid::createUuid().toString(QUuid::WithoutBraces), AcquisitionPhase::Acquiring, {}};
    const auto id = state_.taskId;
    QPointer<FileAcquisition> guard(this);
    service_.acquire(id, root, file, [guard, id](FileResult result) {
        if (!guard || guard->state_.taskId != id ||
            (guard->state_.phase != AcquisitionPhase::Acquiring && guard->state_.phase != AcquisitionPhase::Cancelling)) return;
        guard->state_.result = std::move(result);
        guard->state_.phase = guard->state_.result.error == FileError::None ? AcquisitionPhase::Cached :
            guard->state_.result.error == FileError::Cancelled ? AcquisitionPhase::Cancelled : AcquisitionPhase::Failed;
    });
    return true;
}
bool FileAcquisition::cancel(const QString& id) {
    if (id != state_.taskId || state_.phase != AcquisitionPhase::Acquiring) return false;
    state_.phase = AcquisitionPhase::Cancelling;
    service_.cancel(id);
    return true;
}
}
