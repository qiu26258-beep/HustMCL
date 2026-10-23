#pragma once
#include <hustmcl/core/launch_plan.h>
#include <QObject>
#include <QString>
#include <functional>

namespace hustmcl::application {
enum class FileError { None, InvalidInput, Busy, UnsafePath, Locked, Storage, Network, Timeout, Integrity, Cancelled };
struct FileResult { FileError error = FileError::None; QString cachePath; bool reused = false; bool retryable = false; int retryAfterMs = 0; };
class FileAcquirer {
public:
    using Completion = std::function<void(FileResult)>;
    virtual ~FileAcquirer() = default;
    virtual FileError beginSession(const QString&, int) { return FileError::None; }
    virtual void endSession() {}
    virtual void acquire(const QString& taskId, const QString& dataRoot,
                         const core::DownloadDescriptor& file, Completion completion) = 0;
    virtual void cancel(const QString& taskId) = 0;
};
enum class AcquisitionPhase { Idle, Acquiring, Cancelling, Cached, Cancelled, Failed };
struct AcquisitionSnapshot {
    QString taskId;
    AcquisitionPhase phase = AcquisitionPhase::Idle;
    FileResult result;
};
class FileAcquisition final : public QObject {
public:
    explicit FileAcquisition(FileAcquirer& service, QObject* parent = nullptr);
    ~FileAcquisition() override;
    bool start(const QString& dataRoot, const core::DownloadDescriptor& file);
    bool cancel(const QString& taskId);
    AcquisitionSnapshot snapshot() const { return state_; }
private:
    FileAcquirer& service_;
    AcquisitionSnapshot state_;
};
}
