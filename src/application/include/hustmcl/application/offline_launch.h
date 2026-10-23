#pragma once
#include <hustmcl/core/launch_plan.h>
#include <QString>
#include <QStringList>
#include <variant>

namespace hustmcl::application {
enum class OfflineLaunchError { InvalidPlayerName, DemoPlan };
struct OfflineLaunch {
    QString playerName;
    QString uuid; // Canonical UUID; offline identity, never an authenticated profile.
    QStringList arguments;
};
using OfflineLaunchResult = std::variant<OfflineLaunch, OfflineLaunchError>;
[[nodiscard]] OfflineLaunchResult makeOfflineLaunch(const core::LaunchPlan& plan, const QString& playerName);
}
