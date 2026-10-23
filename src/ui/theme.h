#pragma once

#include <QFont>
#include <QString>

namespace hustmcl::ui::theme {

inline constexpr const char* kPage = "#F4F0E7";
inline constexpr const char* kSurface = "#FFFCF6";
inline constexpr const char* kRail = "#FBF8F1";
inline constexpr const char* kInk = "#23201C";
inline constexpr const char* kMuted = "#6F675D";
inline constexpr const char* kLine = "#DED5C6";
inline constexpr const char* kLineSubtle = "#F0E9DD";
inline constexpr const char* kRed = "#8C1D2F";
inline constexpr const char* kRedDeep = "#6E1524";
inline constexpr const char* kRedSoft = "#F2E3DF";
inline constexpr const char* kMoss = "#4F6B52";
inline constexpr const char* kMossSoft = "#E8F1EC";
inline constexpr const char* kError = "#B3261E";
inline constexpr const char* kFocus = "#1D5FA7";

QFont baseFont();
QString styleSheet();

} // namespace hustmcl::ui::theme
