#include <hustmcl/application/authenticated_launch.h>
namespace hustmcl::application {
AuthenticatedLaunchResult makeAuthenticatedLaunch(const core::LaunchPlan& plan, const MinecraftSession& session) {
    if (!session.valid()) return AuthFailure{AuthError::Expired};
    GameLaunch result{session.playerName, session.uuid, {}};
    result.authenticated = true;
    result.redactions = {session.accessToken};
    for (const auto& argument : plan.arguments) {
        QString value;
        for (const auto& part : argument) {
            if (const auto* literal = std::get_if<std::string>(&part)) value += QString::fromStdString(*literal);
            else switch (std::get<core::SessionField>(part)) {
            case core::SessionField::PlayerName: value += session.playerName; break;
            case core::SessionField::Uuid: value += session.uuid; break;
            case core::SessionField::AccessToken: value += session.accessToken; break;
            case core::SessionField::ClientId: value += session.clientId; break;
            case core::SessionField::Xuid: value += session.xuid; break;
            }
        }
        if (value == "--demo") return AuthFailure{AuthError::NoOwnership};
        result.arguments.append(value);
    }
    return result;
}
}
