#pragma once
#include <hustmcl/core/launch_plan.h>
#include <QString>
#include <QStringList>
#include <variant>

namespace hustmcl::application {
enum class OfflineLaunchError { InvalidPlayerName, DemoPlan };
struct GameLaunch {
    GameLaunch() = default;
    GameLaunch(QString name, QString id, QStringList args)
        : playerName(std::move(name)), uuid(std::move(id)), arguments(std::move(args)) {}
    QString playerName;
    QString uuid;
    QStringList arguments;
    bool authenticated = false;
    QStringList redactions; // Backend-only secrets to remove from process output.
};
// Source compatibility for the existing explicitly offline use case.
using OfflineLaunch = GameLaunch;
using OfflineLaunchResult = std::variant<OfflineLaunch, OfflineLaunchError>;
[[nodiscard]] OfflineLaunchResult makeOfflineLaunch(const core::LaunchPlan& plan, const QString& playerName);
}
