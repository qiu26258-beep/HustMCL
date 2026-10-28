#pragma once
#include <hustmcl/application/account.h>
#include <hustmcl/application/offline_launch.h>
namespace hustmcl::application {
using AuthenticatedLaunchResult = std::variant<GameLaunch, AuthFailure>;
AuthenticatedLaunchResult makeAuthenticatedLaunch(const core::LaunchPlan&, const MinecraftSession&);
}
