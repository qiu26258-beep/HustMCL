#pragma once
#include <hustmcl/application/file_acquisition.h>
#include <hustmcl/core/install_plan.h>
#include <QStringList>
namespace hustmcl::application {
struct BatchSnapshot {
    QString taskId;
    AcquisitionPhase phase = AcquisitionPhase::Idle;
    std::size_t completedFiles = 0, totalFiles = 0, reusedFiles = 0;
    std::uint64_t verifiedBytes = 0, totalBytes = 0;
    FileError error = FileError::None;
};
class BatchAcquisition final : public QObject {
    Q_OBJECT
public:
    explicit BatchAcquisition(FileAcquirer& service, QObject* parent = nullptr, int concurrency = 1);
    ~BatchAcquisition() override;
    bool start(const QString& root, const core::InstallFilePlan& plan);
    bool cancel(const QString& taskId);
    BatchSnapshot snapshot() const { return state_; }
    QStringList cachePaths() const { return paths_; }
signals:
    void changed(const hustmcl::application::BatchSnapshot& snapshot);
private:
    void next(const QString& taskId);
    void publish();
    void stopActive();
    void finish();
    FileAcquirer& service_;
    BatchSnapshot state_;
    QString root_;
    int concurrency_;
    bool sessionOpen_ = false;
    bool shuttingDown_ = false;
    std::map<QString, std::size_t> active_;
    std::vector<bool> dispatched_;
    std::vector<core::PlannedFile> files_;
    QStringList paths_;
};
}
Q_DECLARE_METATYPE(hustmcl::application::BatchSnapshot)
