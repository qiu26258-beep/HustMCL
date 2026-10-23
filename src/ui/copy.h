#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QString>

namespace hustmcl::ui::copy {

QString primaryActionText(application::PrimaryAction action);
QString accountStateText(application::AccountState state);
QString instanceStateText(application::InstanceState state);
QString javaStateText(application::JavaState state);
QString taskStateText(application::TaskState state);
// Shown on the real "实例与检查" page. It deliberately never calls a local
// read-only verification a download or an install.
QString checkPhaseText(application::TaskPhase phase);
QString errorCodeText(application::TaskErrorCode code);
QString formatBytes(qint64 bytes);
QString formatMemory(int memoryMb);
// Shared display normalization for byte counts. The result is in [0, 1].
// Non-positive totals and non-positive completed counts normalize to zero.
double progressFraction(qint64 completedBytes, qint64 totalBytes);
QString formatPercent(qint64 completedBytes, qint64 totalBytes);

} // namespace hustmcl::ui::copy
