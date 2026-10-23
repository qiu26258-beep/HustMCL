#include <hustmcl/application/batch_acquisition.h>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <limits>
#include <map>
namespace hustmcl::application {
BatchAcquisition::BatchAcquisition(FileAcquirer& service, QObject* parent, int concurrency)
    : QObject(parent), service_(service), concurrency_(concurrency) { qRegisterMetaType<BatchSnapshot>(); }
BatchAcquisition::~BatchAcquisition() {
    shuttingDown_ = true;
    for (const auto& [operation, index] : active_) { (void)index; service_.cancel(operation); }
    if (sessionOpen_) service_.endSession();
}
void BatchAcquisition::publish() { const auto copy = state_; emit changed(copy); }
bool BatchAcquisition::start(const QString& root, const core::InstallFilePlan& plan) {
    if (state_.phase == AcquisitionPhase::Acquiring || state_.phase == AcquisitionPhase::Cancelling) return false;
    state_ = {}; paths_.clear(); active_.clear(); files_.clear(); root_ = root;
    state_.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    std::map<std::string, std::uint64_t> sizes;
    bool valid = concurrency_ >= 1 && concurrency_ <= 4 && !root.trimmed().isEmpty() && !plan.files.empty() && plan.files.size() <= 200000;
    for (const auto& file : plan.files) {
        const auto& d = file.download;
        if (d.sha1.size() != 40 || d.sha1.find_first_not_of("0123456789abcdef") != std::string::npos || d.url.empty() ||
            d.size > std::numeric_limits<std::uint64_t>::max() - state_.totalBytes) { valid = false; break; }
        const auto [it, inserted] = sizes.emplace(d.sha1, d.size);
        if (!inserted && it->second != d.size) { valid = false; break; }
        state_.totalBytes += d.size;
    }
    if (state_.totalBytes != plan.totalBytes) valid = false;
    state_.totalFiles = plan.files.size();
    state_.phase = valid ? AcquisitionPhase::Acquiring : AcquisitionPhase::Failed;
    if (!valid) { state_.error = FileError::InvalidInput; publish(); return true; }
    const auto error = service_.beginSession(root, concurrency_);
    if (error != FileError::None) { state_.phase = AcquisitionPhase::Failed; state_.error = error; publish(); return true; }
    sessionOpen_ = true;
    files_ = plan.files;
    dispatched_.assign(files_.size(), false);
    paths_.resize(static_cast<qsizetype>(files_.size()));
    const auto id = state_.taskId;
    QTimer::singleShot(0, this, [this, id] { next(id); });
    publish();
    return true;
}
void BatchAcquisition::stopActive() {
    const auto operations = active_;
    QPointer<BatchAcquisition> guard(this);
    for (const auto& [operation, index] : operations) {
        (void)index;
        service_.cancel(operation);
        if (!guard) return;
    }
}
void BatchAcquisition::finish() {
    if (!active_.empty()) return;
    if (sessionOpen_) { service_.endSession(); sessionOpen_ = false; }
    state_.phase = state_.error == FileError::None ? AcquisitionPhase::Cached :
        state_.error == FileError::Cancelled ? AcquisitionPhase::Cancelled : AcquisitionPhase::Failed;
    publish();
}
bool BatchAcquisition::cancel(const QString& id) {
    if (id != state_.taskId || state_.phase != AcquisitionPhase::Acquiring) return false;
    state_.phase = AcquisitionPhase::Cancelling; state_.error = FileError::Cancelled;
    QPointer<BatchAcquisition> guard(this);
    publish();
    if (!guard) return true;
    if (active_.empty()) finish(); else stopActive();
    return true;
}
void BatchAcquisition::next(const QString& id) {
    if (state_.taskId != id || state_.phase != AcquisitionPhase::Acquiring) return;
    if (state_.completedFiles == files_.size()) { finish(); return; }
    QPointer<BatchAcquisition> guard(this);
    for (std::size_t index = 0; index < files_.size() && active_.size() < static_cast<std::size_t>(concurrency_); ++index) {
        if (dispatched_[index]) continue;
        bool collision = false;
        for (const auto& [operation, other] : active_) { (void)operation; if (files_[other].download.sha1 == files_[index].download.sha1) collision = true; }
        if (collision) continue;
        const auto operation = QUuid::createUuid().toString(QUuid::WithoutBraces);
        dispatched_[index] = true; active_[operation] = index;
        service_.acquire(operation, root_, files_[index].download, [guard, id, operation, index](FileResult result) {
            if (!guard || guard->shuttingDown_ || guard->state_.taskId != id || !guard->active_.contains(operation)) return;
            auto& self = *guard;
            self.active_.erase(operation);
            if (self.state_.phase == AcquisitionPhase::Cancelling) {
                if (self.active_.empty()) self.finish();
                return;
            }
            if (result.error != FileError::None || result.cachePath.isEmpty()) {
                self.state_.error = result.error == FileError::None ? FileError::Storage : result.error;
                self.state_.phase = AcquisitionPhase::Cancelling;
                if (self.active_.empty()) self.finish(); else { self.publish(); if (guard) guard->stopActive(); }
                return;
            }
            self.state_.verifiedBytes += self.files_[index].download.size;
            ++self.state_.completedFiles;
            if (result.reused) ++self.state_.reusedFiles;
            self.paths_[static_cast<qsizetype>(index)] = result.cachePath;
            QTimer::singleShot(0, &self, [guard, id] { if (guard) guard->next(id); });
            self.publish();
        });
        if (!guard || state_.taskId != id || state_.phase != AcquisitionPhase::Acquiring) return;
        // Even synchronous implementations dispatch the next item on the event queue.
        if (!active_.contains(operation)) return;
    }
}
}
