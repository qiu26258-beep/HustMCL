#pragma once

#include <QFont>
#include <QString>

namespace hustmcl::ui::theme {

// Single source for every colour in the UI. Palette follows the product icon
// (resources/brand/hustmcl.png): deep forest green, ivory, grass green, with
// brick red kept for cancel/danger. QSS refers to these by token name
// (see kTokens in theme.cpp); painted widgets use the constants directly.
//
// Contrast (WCAG, computed): ink/page 12.8, muted/page 4.8, muted/surface 5.5,
// onPrimary/primary 5.8, railStrong/rail 10.4, railText/rail 6.7, gold/rail 7.5.

// Paper
inline constexpr const char* kPage = "#F2EDDF";
inline constexpr const char* kSurface = "#FFFDF8";
inline constexpr const char* kSunken = "#F5F1E4";
inline constexpr const char* kInk = "#1B2A24";
inline constexpr const char* kInkSoft = "#3F4F45";
inline constexpr const char* kInkHover = "#2C3D34";
inline constexpr const char* kMuted = "#5E6B62";
inline constexpr const char* kLine = "#DDD8C6";
inline constexpr const char* kLineSubtle = "#EEEADB";
inline constexpr const char* kLineFaint = "#E9E6D8";
inline constexpr const char* kStageLine = "#E3DFCB";
inline constexpr const char* kLinkLine = "#CFCBB8";

// Grass green: the main action and "done" states
inline constexpr const char* kPrimary = "#2A7339";
inline constexpr const char* kPrimaryHover = "#2F8040";
inline constexpr const char* kPrimaryDeep = "#1B4D27";
inline constexpr const char* kPrimarySoft = "#E3EEDC";
inline constexpr const char* kPrimaryTint = "#F1F6EC";
inline constexpr const char* kOk = "#25683A";
inline constexpr const char* kOkSoft = "#E3EEDC";
inline constexpr const char* kMoss = "#3F6B4A";
inline constexpr const char* kCaption = "#2F5E4A";
inline constexpr const char* kDropBg = "#F4F8EE";
inline constexpr const char* kDropLine = "#B9CDB0";
inline constexpr const char* kUnknownText = "#4A5A4E";

// Status
inline constexpr const char* kWarn = "#8A5A00";
inline constexpr const char* kWarnSoft = "#FBF1DC";
inline constexpr const char* kWarnLine = "#E7D3A8";
inline constexpr const char* kError = "#B3261E";
inline constexpr const char* kErrorSoft = "#FBEAE9";
inline constexpr const char* kErrorLine = "#E3B4B0";
inline constexpr const char* kCancel = "#9E3A30";
inline constexpr const char* kCancelLine = "#D7A9A3";
inline constexpr const char* kCancelTint = "#FBF1EF";
inline constexpr const char* kFocus = "#1D5FA7";
inline constexpr const char* kDisabledText = "#7A838D";
inline constexpr const char* kDisabledBg = "#E6E9E1";
inline constexpr const char* kOnPrimary = "#FFFFFF";

// Gold: recommendation and the focus/selection accent on the dark rail
inline constexpr const char* kGold = "#F2C86B";
inline constexpr const char* kGoldSoft = "#F8EBC4";
inline constexpr const char* kGoldText = "#6B4A00";

// Forest rail
inline constexpr const char* kRail = "#0F3F32";
inline constexpr const char* kRailHover = "#17493B";
inline constexpr const char* kRailSelected = "#1F5A47";
inline constexpr const char* kRailStrong = "#F7F0DF";
inline constexpr const char* kRailText = "#B7C7B8";
inline constexpr const char* kRailMuted = "#A9BFAE";
inline constexpr const char* kRailFoot = "#93AD9E";
inline constexpr const char* kRailOff = "#6F8B7D";

QFont baseFont();
QString styleSheet();

} // namespace hustmcl::ui::theme
