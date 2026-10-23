#include <hustmcl/application/batch_acquisition.h>
#include <hustmcl/services/file_acquirer.h>
#include <hustmcl/services/install_plan_input.h>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <iostream>
#include <atomic>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace {
std::atomic_bool stopRequested{false};
BOOL WINAPI consoleControl(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE;
    stopRequested.store(true); return TRUE;
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    auto args = app.arguments();
    int cancelAfterMs = 0;
    for (int i = 1; i < args.size(); ++i) {
        if (!args[i].startsWith("--cancel-after-ms=")) continue;
        bool valid = false; cancelAfterMs = args[i].mid(18).toInt(&valid);
        if (!valid || cancelAfterMs <= 0) return 2;
        args.removeAt(i--);
    }
    if (args.size() != 5 && args.size() != 6) {
        std::cerr << "usage: hustmcl_batch_probe DATA_ROOT METADATA SHA1 ASSET_INDEX [--logging-only] [--cancel-after-ms=N]\n"; return 2;
    }
    if (args.size() == 6 && args[5] != "--logging-only") return 2;
    auto input = hustmcl::services::loadInstallFilePlan(args[2], args[3].toLatin1(), args[4]);
    auto* plan = std::get_if<hustmcl::core::InstallFilePlan>(&input);
    if (!plan) { std::cerr << "Invalid plan input\n"; return 1; }
    if (args.size() == 6) {
        std::erase_if(plan->files, [](const auto& f) { return f.role != hustmcl::core::FileRole::LoggingConfig; });
        plan->totalBytes = 0;
        for (const auto& file : plan->files) plan->totalBytes += file.download.size;
    }
    hustmcl::services::QtFileAcquirer service({120000, false});
    hustmcl::application::BatchAcquisition batch(service, nullptr, 4);
    if (!SetConsoleCtrlHandler(consoleControl, TRUE)) { std::cerr << "Cannot install cancellation handler\n"; return 2; }
    QTimer stopTimer;
    QObject::connect(&stopTimer, &QTimer::timeout, &app, [&] {
        if (stopRequested.load()) batch.cancel(batch.snapshot().taskId);
    });
    stopTimer.start(20);
    if (cancelAfterMs) QTimer::singleShot(cancelAfterMs, &app, [] { stopRequested.store(true); });
    QObject::connect(&batch, &hustmcl::application::BatchAcquisition::changed, &app, [&](const auto& state) {
        using hustmcl::application::AcquisitionPhase;
        if (state.phase == AcquisitionPhase::Acquiring || state.phase == AcquisitionPhase::Cancelling) return;
        QJsonObject result{{"status", state.phase == AcquisitionPhase::Cached ? "cached" : state.phase == AcquisitionPhase::Cancelled ? "cancelled" : "failed"},
            {"fileCount", static_cast<int>(state.totalFiles)}, {"completedFiles", static_cast<int>(state.completedFiles)},
            {"reusedFiles", static_cast<int>(state.reusedFiles)}, {"verifiedBytes", QString::number(state.verifiedBytes)},
            {"error", static_cast<int>(state.error)}, {"installationVerified", false}, {"realGameStarted", false}};
        std::cout << QJsonDocument(result).toJson().toStdString();
        app.exit(state.phase == AcquisitionPhase::Cached ? 0 : state.phase == AcquisitionPhase::Cancelled ? 4 : 1);
    });
    QTimer::singleShot(0, &app, [&] { batch.start(args[1], *plan); });
    const int result = app.exec();
    SetConsoleCtrlHandler(consoleControl, FALSE);
    return result;
}
