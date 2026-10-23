#pragma once

#include <hustmcl/application/launch_preparation.h>

#include <QHash>
#include <QObject>

namespace hustmcl::services {

class QtJavaProbe final : public QObject, public application::JavaRuntimeProbe {
public:
    explicit QtJavaProbe(QObject* parent = nullptr, int timeoutMs = 8000, int maximumOutputBytes = 65536);
    ~QtJavaProbe() override;
    void probe(const QString& operationId, const std::filesystem::path& executable, Completion complete) override;
    void cancel(const QString& operationId) override;

private:
    struct Job;
    void consume(Job* job);
    void stop(Job* job, application::JavaProbeError reason);
    void finish(Job* job, application::JavaProbeResult result);
    QHash<QString, Job*> jobs_;
    int timeoutMs_;
    int maximumOutputBytes_;
};

} // namespace hustmcl::services
