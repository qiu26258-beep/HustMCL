#pragma once

#include <QColor>
#include <QFont>
#include <QString>
#include <QTime>

namespace hustmcl::ui::theme {

// User-selectable theme preference. AutoByTime switches cleanly between
// Birch & Sandstone (Light, 06:00-17:59) and Deepslate Forest (Dark, 18:00-05:59).
enum class ThemeMode {
    AutoByTime,
    Dark,
    Light,
};

enum class ResolvedTheme {
    Dark,
    Light,
};

enum class SkyPhase {
    Dawn,   // 06:00 - 08:59 晨雾
    Day,    // 09:00 - 16:59 晴昼
    Sunset, // 17:00 - 19:29 晚霞
    Night,  // 19:30 - 05:59 星夜
};

struct Palette {
    // Window & Panels (Deepslate Forest / Birch & Sandstone)
    const char* page;
    const char* surface;
    const char* surfaceAlt;
    const char* sunken;
    const char* ink;
    const char* inkSoft;
    const char* muted;
    const char* border;
    const char* bevelHi;
    const char* bevelLo;
    const char* lineSubtle;

    // Pixel Titlebar
    const char* titleBar;
    const char* titleBarBorder;
    const char* titleText;
    const char* titleMuted;
    const char* titleBtnHover;

    // Forest Hotbar Sidebar Rail
    const char* rail;
    const char* railBorder;
    const char* railHover;
    const char* railSelected;
    const char* railStrong;
    const char* railText;
    const char* railMuted;
    const char* railFoot;
    const char* railOff;

    // Emerald Grass Primary CTA & Ready States
    const char* primary;
    const char* primaryHover;
    const char* primaryHi;
    const char* primaryLo;
    const char* primaryDeep;
    const char* primarySoft;
    const char* okText;
    const char* onPrimary;

    // HUST South Gate Crimson (Danger / Cancel / Brand Accent)
    const char* crimson;
    const char* crimsonHover;
    const char* crimsonHi;
    const char* crimsonLo;
    const char* crimsonSoft;
    const char* crimsonLine;

    // Sycamore Gold (Recommended / Warning / Enchant Focus)
    const char* gold;
    const char* goldSoft;
    const char* goldLine;
    const char* goldText;

    // Lapis Blue (Market / Info / Focus)
    const char* focus;
    const char* lapis;
    const char* lapisSoft;

    // Disabled
    const char* disabledBg;
    const char* disabledText;
    const char* disabledBorder;

    // Dropzone & XP Bar
    const char* dropBg;
    const char* dropLine;
    const char* xpTrack;
    const char* xpBorder;
};

ResolvedTheme resolveTheme(ThemeMode mode, const QTime& localTime = QTime::currentTime());
SkyPhase resolveSkyPhase(const QTime& localTime = QTime::currentTime());
int msecsUntilNextBoundary(const QTime& localTime = QTime::currentTime());

QString themeModeKey(ThemeMode mode);
ThemeMode themeModeFromKey(const QString& key);
QString themeModeLabel(ThemeMode mode);
QString resolvedThemeLabel(ResolvedTheme theme);
QString skyPhaseLabel(SkyPhase phase);

const Palette& palette(ResolvedTheme theme);
const Palette& currentPalette();
ResolvedTheme activeTheme();
void setActiveTheme(ResolvedTheme theme);

QColor color(const char* hex);
QFont baseFont();
QFont monoFont(qreal pointSize = 9.5);
QString styleSheet(ResolvedTheme theme);
QString styleSheet();

} // namespace hustmcl::ui::theme
