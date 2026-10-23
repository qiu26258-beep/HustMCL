#include <hustmcl/application/batch_acquisition.h>
#include <QCoreApplication>
#include <cstdlib>
#include <iostream>
using namespace hustmcl;
using namespace hustmcl::application;
void check(bool ok) { if (!ok) std::abort(); }
struct Fake : FileAcquirer {
    Completion callback;
    int calls = 0, cancels = 0;
    bool synchronous = false;
    void acquire(const QString&, const QString&, const core::DownloadDescriptor&, Completion done) override {
        ++calls; callback = done;
        if (synchronous) done({FileError::None, "cached", true});
    }
    void cancel(const QString&) override { ++cancels; }
};
void pump() { QCoreApplication::processEvents(); }
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    core::InstallFilePlan plan;
    plan.files = {{"a", {"https://example.test/a", std::string(40, 'a'), 3}, core::FileRole::Library},
                  {"b", {"https://example.test/b", std::string(40, 'b'), 5}, core::FileRole::Library}};
    plan.totalBytes = 8;
    Fake fake; BatchAcquisition batch(fake);
    check(batch.start("root", plan)); pump(); check(fake.calls == 1);
    auto old = fake.callback;
    fake.callback({FileError::None, "a", false});
    old({FileError::None, "duplicate", false}); pump();
    check(fake.calls == 2 && batch.snapshot().verifiedBytes == 3);
    fake.callback({FileError::Network, {}, false}); pump();
    check(batch.snapshot().phase == AcquisitionPhase::Failed && fake.calls == 2);
    check(batch.start("root", plan)); pump();
    old({FileError::None, "stale", false}); check(batch.snapshot().completedFiles == 0);
    fake.callback({FileError::None, "a", true}); pump();
    fake.callback({FileError::None, "b", false}); pump();
    check(batch.snapshot().phase == AcquisitionPhase::Cached && batch.snapshot().verifiedBytes == 8 && batch.snapshot().reusedFiles == 1 && batch.cachePaths().size() == 2);
    check(batch.start("root", plan)); pump();
    check(batch.cancel(batch.snapshot().taskId)); check(batch.snapshot().phase == AcquisitionPhase::Cancelling);
    check(!batch.start("root", plan));
    fake.callback({FileError::Cancelled, {}, false}); pump();
    check(batch.snapshot().phase == AcquisitionPhase::Cancelled && fake.cancels == 1);
    const auto calls = fake.calls;
    check(batch.start("root", plan)); check(batch.cancel(batch.snapshot().taskId)); pump(); check(fake.calls == calls);
    auto invalid = plan; invalid.totalBytes++;
    check(batch.start("root", invalid)); pump(); check(fake.calls == calls && batch.snapshot().error == FileError::InvalidInput);
    invalid = plan; invalid.files[1].download.sha1 = invalid.files[0].download.sha1;
    check(batch.start("root", invalid)); pump(); check(fake.calls == calls && batch.snapshot().phase == AcquisitionPhase::Failed);
    fake.synchronous = true;
    plan.files.assign(5000, plan.files.front()); plan.totalBytes = 15000;
    check(batch.start("root", plan));
    for (int i = 0; i < 6000 && batch.snapshot().phase == AcquisitionPhase::Acquiring; ++i) pump();
    check(batch.snapshot().phase == AcquisitionPhase::Cached && batch.snapshot().completedFiles == 5000);
    std::cout << "batch acquisition scenarios passed\n";
}
