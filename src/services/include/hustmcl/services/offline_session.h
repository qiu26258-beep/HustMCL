#pragma once
#include <hustmcl/application/offline_controller.h>
#include <QThreadPool>
#include <QProcess>
#include <QLockFile>
#include <QTimer>
#include <QFile>
#include <memory>

namespace hustmcl::services {
class LocalGameInputSource final : public QObject, public application::LaunchInputSource {
public:
    LocalGameInputSource(QString root, QString version, QString java, QObject* parent = nullptr);
    ~LocalGameInputSource() override;
    void load(const QString& instanceId, Completion complete) override;
    void selectInstance(const application::InstalledInstance& instance);
private:
    QString root_, version_, java_;
    QString instanceId_;
    bool imported_ = false;
    QThreadPool workers_;
};
class QtGameProcess final : public QObject, public application::GameProcess {
public:
    explicit QtGameProcess(QString root, QObject* parent = nullptr);
    bool selectRoot(const QString& root);
    void start(const QString& taskId, const core::LaunchPlan& plan,
               const application::OfflineLaunch& identity, Observer observer) override;
private:
    void finish(bool normal, int exitCode);
    void fail(application::GameProcessError error);
    void captureOutput(bool standardError, bool final = false);
    QString root_, logBase_;
    QProcess game_;
    QTimer startTimeout_;
    std::unique_ptr<QLockFile> lock_;
    Observer observer_;
    application::OfflineLaunch identity_;
    QFile stdoutLog_, stderrLog_;
    QByteArray stdoutBuffer_, stderrBuffer_;
    bool logFailed_ = false;
};
}
