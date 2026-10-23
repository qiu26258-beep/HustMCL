#include <hustmcl/application/offline_launch.h>
#include <iostream>
#include <cstdlib>
using namespace hustmcl;
void check(bool ok, const char* message) { if (!ok) { std::cerr << message << '\n'; std::exit(1); } }
int main() {
    core::LaunchPlan plan;
    plan.arguments = {{std::string("-cp")}, {std::string("D:/space path/中文.jar")},
        {core::SessionField::PlayerName}, {core::SessionField::Uuid},
        {core::SessionField::AccessToken}, {core::SessionField::ClientId}, {core::SessionField::Xuid}};
    auto result = application::makeOfflineLaunch(plan, "Notch");
    check(std::holds_alternative<application::OfflineLaunch>(result), "offline identity");
    const auto& launch = std::get<application::OfflineLaunch>(result);
    check(launch.uuid == "b50ad385-829d-3141-a216-7e7d7539ba7f", "independent v3 UUID fixture");
    check(launch.arguments == QStringList{"-cp", QString::fromUtf8("D:/space path/中文.jar"), "Notch",
        "b50ad385829d3141a2167e7d7539ba7f", "0", "0", "0"}, "typed session substitution preserves argument boundaries");
    const auto upper = std::get<application::OfflineLaunch>(application::makeOfflineLaunch(plan, "HustPlayer"));
    const auto lower = std::get<application::OfflineLaunch>(application::makeOfflineLaunch(plan, "hustplayer"));
    check(upper.uuid == "8b7e8ac7-95f3-39eb-b631-dc3156bcd130" && lower.uuid == "6dfdd259-86fa-317f-8d45-ab1900d59861",
        "stable, case-sensitive player identity");
    for (const auto* name : {"", "two names", "a;cmd", "abcdefghijklmnopq", "--username"})
        check(std::holds_alternative<application::OfflineLaunchError>(application::makeOfflineLaunch(plan, name)), "invalid name rejected");
    check(std::holds_alternative<application::OfflineLaunchError>(application::makeOfflineLaunch(plan, QString::fromUtf8("华科"))), "ASCII identity contract");
    plan.arguments.push_back({std::string("--demo")});
    check(std::get<application::OfflineLaunchError>(application::makeOfflineLaunch(plan, "HustPlayer")) == application::OfflineLaunchError::DemoPlan,
        "demo plan cannot masquerade as offline full mode");
    std::cout << "Offline identity and argument policy passed\n";
}
