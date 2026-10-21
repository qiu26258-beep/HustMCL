#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QString>

namespace hustmcl::ui::copy {

QString primaryActionText(application::PrimaryAction action);
QString accountStateText(application::AccountState state);
QString instanceStateText(application::InstanceState state);
QString javaStateText(application::JavaState state);
QString phaseTitle(application::TaskPhase phase);
QString errorCodeText(application::TaskErrorCode code);
QString formatBytes(qint64 bytes);
QString formatMemory(int memoryMb);
QString formatPercent(qint64 completedBytes, qint64 totalBytes);

} // namespace hustmcl::ui::copy
