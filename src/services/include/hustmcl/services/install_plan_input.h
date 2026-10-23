#pragma once

#include <hustmcl/application/launch_preparation.h>
#include <hustmcl/core/install_plan.h>

#include <QByteArray>
#include <QString>

namespace hustmcl::services {
using InstallPlanInputResult = std::variant<core::InstallFilePlan, application::PreparationError>;

// Synchronous, bounded local reads for the developer CLI/worker thread, never the GUI event thread.
// No game/cache/instance writes and no readiness claim.
[[nodiscard]] InstallPlanInputResult loadInstallFilePlan(
    const QString& versionFile, const QByteArray& versionSha1, const QString& assetIndexFile);
} // namespace hustmcl::services
