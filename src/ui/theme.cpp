#include "theme.h"

#include <QFontDatabase>
#include <QStringList>

namespace hustmcl::ui::theme {

namespace {

// Scheme A: 喻园深林 · Deepslate Forest (Hardcore Dark Voxel)
constexpr Palette kDarkPalette = {
    // Window & Panels
    "#101916", // page (deep forest deepslate)
    "#16231F", // surface (mossy slate block)
    "#1E302A", // surfaceAlt (raised stone brick)
    "#0B1310", // sunken (recessed bedrock slot)
    "#F3EFE6", // ink (magnolia warm white)
    "#C8D6CD", // inkSoft
    "#8BA396", // muted
    "#2A4439", // border
    "#3E6354", // bevelHi
    "#080F0C", // bevelLo
    "#1E322B", // lineSubtle

    // Pixel Titlebar
    "#09110E", // titleBar
    "#20362D", // titleBarBorder
    "#F5F1E8", // titleText
    "#8AA697", // titleMuted
    "#1A2F27", // titleBtnHover

    // Forest Hotbar Sidebar Rail
    "#0B1411", // rail
    "#21382F", // railBorder
    "#14251F", // railHover
    "#1B362B", // railSelected
    "#F6F2E8", // railStrong
    "#BCD0C4", // railText
    "#8BA697", // railMuted
    "#6E8B7C", // railFoot
    "#4B6458", // railOff

    // Emerald Grass Primary CTA
    "#2FB557", // primary
    "#39C964", // primaryHover
    "#64E88B", // primaryHi
    "#196E33", // primaryLo
    "#0E451F", // primaryDeep
    "#173324", // primarySoft
    "#62E386", // okText
    "#FFFFFF", // onPrimary

    // HUST South Gate Crimson
    "#C94530", // crimson
    "#DE5640", // crimsonHover
    "#F07A67", // crimsonHi
    "#7E2315", // crimsonLo
    "#2E1715", // crimsonSoft
    "#7A332A", // crimsonLine

    // Sycamore Gold
    "#E8B949", // gold
    "#2F2613", // goldSoft
    "#6E541E", // goldLine
    "#F7D47A", // goldText

    // Lapis Focus
    "#58A6FF", // focus
    "#4C8BF5", // lapis
    "#16263E", // lapisSoft

    // Disabled
    "#16201D", // disabledBg
    "#5E7368", // disabledText
    "#22332C", // disabledBorder

    // Dropzone & XP Bar
    "#0D1914", // dropBg
    "#34A857", // dropLine
    "#08100C", // xpTrack
    "#2C473C", // xpBorder
};

// Scheme B: 梧桐晴昼 · Birch & Sandstone (Hardcore Daylight Voxel)
constexpr Palette kLightPalette = {
    // Window & Panels
    "#DCE4DB", // page (clean pale mossy stone)
    "#F4F7F2", // surface (white sandstone block)
    "#E8EFE6", // surfaceAlt (raised birch/limestone)
    "#D7E0D5", // sunken (recessed quarry slot)
    "#15241E", // ink
    "#30463D", // inkSoft
    "#50665C", // muted
    "#9CB0A0", // border
    "#FFFFFF", // bevelHi
    "#809686", // bevelLo
    "#CBD6C9", // lineSubtle

    // Pixel Titlebar (deep HUST forest bar anchored cleanly on top)
    "#102B22", // titleBar
    "#224A3B", // titleBarBorder
    "#F5F1E8", // titleText
    "#9DB8AA", // titleMuted
    "#1C4235", // titleBtnHover

    // Forest Hotbar Sidebar Rail
    "#122F25", // rail
    "#244D3E", // railBorder
    "#1A3F32", // railHover
    "#225442", // railSelected
    "#F7F3EA", // railStrong
    "#C7D9CE", // railText
    "#9CB7A8", // railMuted
    "#82A191", // railFoot
    "#5E7C6E", // railOff

    // Emerald Grass Primary CTA
    "#269B49", // primary
    "#2FB056", // primaryHover
    "#59D47C", // primaryHi
    "#15642D", // primaryLo
    "#0E451F", // primaryDeep
    "#D8F0DF", // primarySoft
    "#1A6E34", // okText
    "#FFFFFF", // onPrimary

    // HUST South Gate Crimson
    "#BF3B26", // crimson
    "#D44B35", // crimsonHover
    "#EB725E", // crimsonHi
    "#7A1D10", // crimsonLo
    "#F9E4E0", // crimsonSoft
    "#D68F84", // crimsonLine

    // Sycamore Gold
    "#D49B1A", // gold
    "#FBF0D2", // goldSoft
    "#D6B462", // goldLine
    "#754F00", // goldText

    // Lapis Focus
    "#1D5FA7", // focus
    "#2E6BD6", // lapis
    "#E1ECFC", // lapisSoft

    // Disabled
    "#DDE4DC", // disabledBg
    "#73857C", // disabledText
    "#B6C3B7", // disabledBorder

    // Dropzone & XP Bar
    "#E6F1E6", // dropBg
    "#2A8F4A", // dropLine
    "#121E19", // xpTrack
    "#5B7366", // xpBorder
};

ResolvedTheme g_activeTheme = ResolvedTheme::Dark;

struct Token {
    const char* name;
    const char* value;
};

} // namespace

ResolvedTheme resolveTheme(ThemeMode mode, const QTime& localTime)
{
    if (mode == ThemeMode::Dark) {
        return ResolvedTheme::Dark;
    }
    if (mode == ThemeMode::Light) {
        return ResolvedTheme::Light;
    }
    const QTime t = localTime.isValid() ? localTime : QTime::currentTime();
    const int hour = t.hour();
    return (hour >= 6 && hour < 18) ? ResolvedTheme::Light : ResolvedTheme::Dark;
}

SkyPhase resolveSkyPhase(const QTime& localTime)
{
    const QTime t = localTime.isValid() ? localTime : QTime::currentTime();
    const int mins = t.hour() * 60 + t.minute();
    if (mins >= 6 * 60 && mins < 9 * 60) {
        return SkyPhase::Dawn;
    }
    if (mins >= 9 * 60 && mins < 17 * 60) {
        return SkyPhase::Day;
    }
    if (mins >= 17 * 60 && mins < 19 * 60 + 30) {
        return SkyPhase::Sunset;
    }
    return SkyPhase::Night;
}

int msecsUntilNextBoundary(const QTime& localTime)
{
    const QTime t = localTime.isValid() ? localTime : QTime::currentTime();
    const int nowSec = t.hour() * 3600 + t.minute() * 60 + t.second();
    // Boundaries: 06:00 (Light+Dawn), 09:00 (Day), 17:00 (Sunset), 18:00 (Dark), 19:30 (Night)
    constexpr int kBoundaries[] = {
        6 * 3600,
        9 * 3600,
        17 * 3600,
        18 * 3600,
        19 * 3600 + 30 * 60,
    };
    int deltaSec = -1;
    for (int boundary : kBoundaries) {
        if (boundary > nowSec) {
            deltaSec = boundary - nowSec;
            break;
        }
    }
    if (deltaSec <= 0) {
        deltaSec = (24 * 3600 - nowSec) + kBoundaries[0];
    }
    // Add 1 second cushion so the clock has crossed the boundary when fired.
    return (deltaSec + 1) * 1000;
}

QString themeModeKey(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::AutoByTime:
        return QStringLiteral("auto");
    case ThemeMode::Dark:
        return QStringLiteral("dark");
    case ThemeMode::Light:
        return QStringLiteral("light");
    }
    return QStringLiteral("auto");
}

ThemeMode themeModeFromKey(const QString& key)
{
    const QString lowered = key.trimmed().toLower();
    if (lowered == QStringLiteral("dark")) {
        return ThemeMode::Dark;
    }
    if (lowered == QStringLiteral("light")) {
        return ThemeMode::Light;
    }
    return ThemeMode::AutoByTime;
}

QString themeModeLabel(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::AutoByTime:
        return QStringLiteral("随时间自动切换");
    case ThemeMode::Dark:
        return QStringLiteral("喻园深林（深色）");
    case ThemeMode::Light:
        return QStringLiteral("梧桐晴昼（浅色）");
    }
    return QStringLiteral("随时间自动切换");
}

QString resolvedThemeLabel(ResolvedTheme theme)
{
    return theme == ResolvedTheme::Dark
        ? QStringLiteral("喻园深林")
        : QStringLiteral("梧桐晴昼");
}

QString skyPhaseLabel(SkyPhase phase)
{
    switch (phase) {
    case SkyPhase::Dawn:
        return QStringLiteral("喻园晨雾");
    case SkyPhase::Day:
        return QStringLiteral("森林晴昼");
    case SkyPhase::Sunset:
        return QStringLiteral("东湖晚霞");
    case SkyPhase::Night:
        return QStringLiteral("喻家星夜");
    }
    return QStringLiteral("森林晴昼");
}

const Palette& palette(ResolvedTheme theme)
{
    return theme == ResolvedTheme::Dark ? kDarkPalette : kLightPalette;
}

const Palette& currentPalette()
{
    return palette(g_activeTheme);
}

ResolvedTheme activeTheme()
{
    return g_activeTheme;
}

void setActiveTheme(ResolvedTheme theme)
{
    g_activeTheme = theme;
}

QColor color(const char* hex)
{
    return QColor(QString::fromLatin1(hex));
}

QFont baseFont()
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSizeF(9.5);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont monoFont(qreal pointSize)
{
    static const QString family = []() {
        const QStringList candidates = {
            QStringLiteral("Cascadia Mono"),
            QStringLiteral("Consolas"),
            QStringLiteral("Courier New"),
        };
        const QStringList installed = QFontDatabase::families();
        for (const QString& candidate : candidates) {
            if (installed.contains(candidate, Qt::CaseInsensitive)) {
                return candidate;
            }
        }
        return QStringLiteral("Consolas");
    }();
    QFont font(family);
    font.setPointSizeF(pointSize);
    font.setStyleHint(QFont::Monospace);
    return font;
}

QString styleSheet()
{
    return styleSheet(g_activeTheme);
}

QString styleSheet(ResolvedTheme theme)
{
    const Palette& p = palette(theme);
    const Token tokens[] = {
        {"page", p.page}, {"surface", p.surface}, {"surfaceAlt", p.surfaceAlt},
        {"sunken", p.sunken}, {"ink", p.ink}, {"inkSoft", p.inkSoft}, {"muted", p.muted},
        {"border", p.border}, {"bevelHi", p.bevelHi}, {"bevelLo", p.bevelLo},
        {"lineSubtle", p.lineSubtle}, {"titleBar", p.titleBar},
        {"titleBarBorder", p.titleBarBorder}, {"titleText", p.titleText},
        {"titleMuted", p.titleMuted}, {"titleBtnHover", p.titleBtnHover},
        {"rail", p.rail}, {"railBorder", p.railBorder}, {"railHover", p.railHover},
        {"railSelected", p.railSelected}, {"railStrong", p.railStrong},
        {"railText", p.railText}, {"railMuted", p.railMuted}, {"railFoot", p.railFoot},
        {"railOff", p.railOff}, {"primary", p.primary}, {"primaryHover", p.primaryHover},
        {"primaryHi", p.primaryHi}, {"primaryLo", p.primaryLo}, {"primaryDeep", p.primaryDeep},
        {"primarySoft", p.primarySoft}, {"okText", p.okText}, {"onPrimary", p.onPrimary},
        {"crimson", p.crimson}, {"crimsonHover", p.crimsonHover}, {"crimsonHi", p.crimsonHi},
        {"crimsonLo", p.crimsonLo}, {"crimsonSoft", p.crimsonSoft}, {"crimsonLine", p.crimsonLine},
        {"gold", p.gold}, {"goldSoft", p.goldSoft}, {"goldLine", p.goldLine},
        {"goldText", p.goldText}, {"focus", p.focus}, {"lapis", p.lapis},
        {"lapisSoft", p.lapisSoft}, {"disabledBg", p.disabledBg},
        {"disabledText", p.disabledText}, {"disabledBorder", p.disabledBorder},
        {"dropBg", p.dropBg}, {"dropLine", p.dropLine}, {"xpTrack", p.xpTrack},
        {"xpBorder", p.xpBorder},
    };

    QString qss = QStringLiteral(R"QSS(
QWidget {
    color: @ink@;
}
QWidget#Root {
    background: @page@;
    border: 2px solid @border@;
}

/* ==================== PIXEL TITLE BAR ==================== */
QFrame#PixelTitleBar {
    background: @titleBar@;
    border: none;
    border-bottom: 2px solid @titleBarBorder@;
}
QLabel#TitleBarText {
    color: @titleText@;
    font-size: 13px;
    font-weight: 700;
}
QLabel#TitleBarSub {
    color: @titleMuted@;
    font-size: 11px;
}
QLabel#TitleBarCoord {
    color: @titleMuted@;
    background: @railHover@;
    border: 1px solid @railBorder@;
    padding: 2px 7px;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 10.5px;
}
QPushButton#TitleThemeButton {
    min-height: 24px;
    padding: 0 9px;
    background: @railHover@;
    color: @titleText@;
    border-top: 2px solid @railBorder@;
    border-left: 2px solid @railBorder@;
    border-right: 2px solid #050A08;
    border-bottom: 2px solid #050A08;
    border-radius: 0px;
    font-size: 11px;
    font-weight: 600;
}
QPushButton#TitleThemeButton:hover {
    background: @railSelected@;
    border-top-color: @primaryHi@;
    border-left-color: @primaryHi@;
}
QPushButton#TitleThemeButton:focus {
    border: 2px solid @gold@;
}
QPushButton#TitleWinButton, QPushButton#TitleCloseButton {
    min-width: 34px;
    max-width: 34px;
    min-height: 26px;
    max-height: 26px;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 0px;
    color: @titleText@;
}
QPushButton#TitleWinButton:hover {
    background: @titleBtnHover@;
    border-top: 2px solid @railBorder@;
    border-left: 2px solid @railBorder@;
    border-right: 2px solid #050A08;
    border-bottom: 2px solid #050A08;
}
QPushButton#TitleCloseButton:hover {
    background: @crimson@;
    border-top: 2px solid @crimsonHi@;
    border-left: 2px solid @crimsonHi@;
    border-right: 2px solid @crimsonLo@;
    border-bottom: 2px solid @crimsonLo@;
}

/* ==================== FOREST HOTBAR SIDEBAR ==================== */
QWidget#Sidebar {
    background: @rail@;
    border-right: 2px solid @railBorder@;
}
QFrame#BrandBlock {
    background: @railHover@;
    border-top: 2px solid @railBorder@;
    border-left: 2px solid @railBorder@;
    border-right: 2px solid #050A08;
    border-bottom: 2px solid #050A08;
}
QLabel#BrandMark {
    min-width: 38px;
    max-width: 38px;
    min-height: 38px;
    max-height: 38px;
    background: transparent;
    color: @railStrong@;
    font-size: 16px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#BrandName {
    color: @railStrong@;
    font-size: 16px;
    font-weight: 700;
}
QLabel#BrandSub {
    color: @railMuted@;
    font-size: 10.5px;
}
QLabel#DemoBadge {
    color: @gold@;
    background: @railHover@;
    border: 2px solid @railBorder@;
    border-left: 4px solid @primary@;
    border-radius: 0px;
    padding: 4px 8px;
    font-size: 11px;
    font-weight: 600;
}
QLabel#DemoNote {
    color: @railMuted@;
    font-size: 10.5px;
}
QLabel#NavNote {
    color: @railMuted@;
    font-size: 10.5px;
}
QFrame#MottoSeal {
    background: @railHover@;
    border: 1px solid @railBorder@;
}
QLabel#MottoSealText {
    color: @railStrong@;
    font-size: 11px;
    font-weight: 600;
}
QLabel#MottoSealSub {
    color: @railFoot@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 9.5px;
}
QFrame#AccountBlock {
    background: @railHover@;
    border-top: 2px solid @railBorder@;
    border-left: 2px solid @railBorder@;
    border-right: 2px solid #050A08;
    border-bottom: 2px solid #050A08;
}
QLabel#AccountName {
    color: @railStrong@;
    font-size: 12.5px;
    font-weight: 700;
}
QLabel#AccountState {
    color: @railMuted@;
    font-size: 10.5px;
}
QLabel#AccountState[ok="true"] {
    color: @primaryHi@;
}
QLabel#Avatar {
    min-width: 34px;
    max-width: 34px;
    min-height: 34px;
    max-height: 34px;
    border-radius: 0px;
    background: @railSelected@;
    border: 2px solid @primary@;
    color: @railStrong@;
    font-size: 13px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#RailFoot {
    color: @railFoot@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 10px;
}
QFrame#Divider {
    background: @border@;
    border: none;
    max-height: 2px;
}
QWidget#Sidebar QFrame#Divider {
    background: @railBorder@;
    max-height: 2px;
}
QFrame#VDivider {
    background: @border@;
    border: none;
    max-width: 2px;
}
QWidget#Sidebar QPushButton#SecondaryAction {
    color: @railText@;
    border: 1px solid transparent;
    border-bottom: 2px solid @railBorder@;
    font-size: 11.5px;
}
QWidget#Sidebar QPushButton#SecondaryAction:hover {
    color: @railStrong@;
    border-bottom-color: @primaryHi@;
}
QWidget#Sidebar QPushButton#SecondaryAction:focus {
    border: 2px solid @gold@;
}

/* Hotbar Navigation List */
QListWidget#Navigation {
    background: transparent;
    border: none;
    outline: none;
    font-size: 13.5px;
    color: @railText@;
}
QListWidget#Navigation::item {
    height: 44px;
    padding-left: 10px;
    margin-bottom: 3px;
    border-radius: 0px;
    border: 2px solid transparent;
    border-left: 4px solid transparent;
}
QListWidget#Navigation::item:hover {
    background: @railHover@;
    color: @railStrong@;
    border-color: @railBorder@;
    border-left: 4px solid @railBorder@;
}
QListWidget#Navigation::item:selected {
    background: @railSelected@;
    color: @railStrong@;
    border-top: 2px solid @railBorder@;
    border-right: 2px solid #050A08;
    border-bottom: 2px solid #050A08;
    border-left: 4px solid @primaryHi@;
    font-weight: 700;
}
QListWidget#Navigation::item:disabled {
    color: @railOff@;
}
QListWidget#Navigation:focus {
    border: 2px solid @gold@;
}

/* ==================== PAGE SCAFFOLD & VOXEL CARDS ==================== */
QLabel#PageTitle {
    color: @ink@;
    font-size: 24px;
    font-weight: 700;
}
QLabel#PageSubtitle {
    color: @muted@;
    font-size: 12px;
}
QLabel#HeaderMeta {
    color: @muted@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11.5px;
    background: @surfaceAlt@;
    border: 2px solid @border@;
    padding: 3px 8px;
}
QFrame#Desk {
    background: @surface@;
    border-top: 2px solid @bevelHi@;
    border-left: 2px solid @bevelHi@;
    border-right: 2px solid @bevelLo@;
    border-bottom: 3px solid @bevelLo@;
    border-radius: 0px;
}
QFrame#VoxelCard, QFrame#ModpackSection {
    background: @surfaceAlt@;
    border-top: 2px solid @bevelHi@;
    border-left: 2px solid @bevelHi@;
    border-right: 2px solid @bevelLo@;
    border-bottom: 3px solid @bevelLo@;
    border-radius: 0px;
}
QFrame#VoxelSlot {
    background: @sunken@;
    border-top: 2px solid @bevelLo@;
    border-left: 2px solid @bevelLo@;
    border-right: 2px solid @bevelHi@;
    border-bottom: 2px solid @bevelHi@;
    border-radius: 0px;
}
QScrollArea#DeskScroll {
    border: none;
    background: transparent;
}
QScrollArea#DeskScroll > QWidget > QWidget {
    background: transparent;
}
QWidget#DeskContent {
    background: transparent;
}

/* ==================== TYPOGRAPHY & STATUS BLOCKS ==================== */
QLabel#InstanceName {
    color: @ink@;
    font-size: 20px;
    font-weight: 700;
}
QLabel#InstanceMeta {
    color: @muted@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 12px;
}
QLabel#SectionTitle {
    color: @ink@;
    font-size: 13px;
    font-weight: 700;
}
QLabel#StatusCheck {
    min-width: 24px;
    max-width: 24px;
    min-height: 24px;
    max-height: 24px;
    border-radius: 0px;
    background: @primarySoft@;
    border: 2px solid @primary@;
    color: @okText@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 12px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#StatusCheck[ok="false"] {
    background: @goldSoft@;
    border: 2px solid @gold@;
    color: @goldText@;
}
QLabel#StatusName {
    color: @ink@;
    font-size: 12.5px;
    font-weight: 700;
}
QLabel#StatusValue {
    color: @inkSoft@;
    font-size: 12px;
}
QLabel#StatusState {
    color: @okText@;
    font-size: 11px;
    font-weight: 700;
}
QLabel#StatusState[ok="false"] {
    color: @goldText@;
}
QLabel#SummaryLabel {
    color: @muted@;
    font-size: 12px;
}
QLabel#SummaryValue {
    color: @ink@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 12px;
    font-weight: 700;
}
QLabel#Footnote {
    color: @muted@;
    font-size: 11.5px;
}
QLabel#InstallPath, QLabel#PathValue {
    color: @inkSoft@;
    background: @sunken@;
    border-top: 2px solid @bevelLo@;
    border-left: 2px solid @bevelLo@;
    border-right: 2px solid @bevelHi@;
    border-bottom: 2px solid @bevelHi@;
    border-radius: 0px;
    padding: 6px 9px;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11px;
}
QLabel#LogStatusTitle {
    color: @ink@;
    font-size: 17px;
    font-weight: 700;
}
QLabel#CheckDetail {
    color: @muted@;
    font-size: 12px;
}
QLabel#FieldError, QLabel#LoginModeError {
    color: @crimsonHi@;
    background: @crimsonSoft@;
    border: 2px solid @crimsonLine@;
    padding: 6px 8px;
    font-size: 12px;
}
QLabel#FieldSuccess {
    color: @okText@;
    background: @primarySoft@;
    border: 2px solid @primary@;
    padding: 6px 8px;
    font-size: 12px;
}
QLabel#SettingsAccountName {
    color: @ink@;
    font-size: 15px;
    font-weight: 700;
}
QLabel#SettingsAccountState {
    color: @okText@;
    font-size: 12px;
    font-weight: 600;
}
QLabel#AccountMessage {
    color: @inkSoft@;
    background: @sunken@;
    border: 2px solid @border@;
    border-radius: 0px;
    padding: 6px 9px;
    font-size: 12px;
}
QLabel#AccountMessage[error="true"] {
    color: @crimsonHi@;
    background: @crimsonSoft@;
    border: 2px solid @crimsonLine@;
}

/* ==================== INPUTS, COMBOS, SPINBOX & PRESET CHIPS ==================== */
QLineEdit#PlayerNameInput,
QLineEdit#MarketSearchInput,
QLineEdit#MarketVersionInput,
QSpinBox#MemoryInput {
    min-height: 34px;
    padding: 0 9px;
    background: @sunken@;
    border-top: 2px solid @bevelLo@;
    border-left: 2px solid @bevelLo@;
    border-right: 2px solid @bevelHi@;
    border-bottom: 2px solid @bevelHi@;
    border-radius: 0px;
    color: @ink@;
    font-size: 12.5px;
    selection-background-color: @primarySoft@;
    selection-color: @ink@;
}
QSpinBox#MemoryInput {
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-weight: 700;
}
QSpinBox#MemoryInput::up-button, QSpinBox#MemoryInput::down-button {
    width: 22px;
    background: @surfaceAlt@;
    border-left: 2px solid @border@;
}
QLineEdit#MarketVersionInput {
    min-width: 146px;
    max-width: 146px;
    font-family: "Cascadia Mono", "Consolas", monospace;
}
QLineEdit#PlayerNameInput:focus,
QLineEdit#MarketSearchInput:focus,
QLineEdit#MarketVersionInput:focus,
QSpinBox#MemoryInput:focus {
    border: 2px solid @focus@;
}
QLineEdit#PlayerNameInput:disabled,
QLineEdit#MarketSearchInput:disabled,
QLineEdit#MarketVersionInput:disabled,
QSpinBox#MemoryInput:disabled {
    background: @disabledBg@;
    color: @disabledText@;
    border-color: @disabledBorder@;
}

QComboBox#LoginModeSelector,
QComboBox#ThemeModeSelector,
QComboBox#MarketLoader,
QComboBox#VanillaVersionCombo,
QComboBox#ModpackInstanceCombo {
    min-height: 34px;
    padding: 0 26px 0 9px;
    background: @sunken@;
    border-top: 2px solid @bevelLo@;
    border-left: 2px solid @bevelLo@;
    border-right: 2px solid @bevelHi@;
    border-bottom: 2px solid @bevelHi@;
    border-radius: 0px;
    color: @ink@;
    font-size: 12.5px;
}
QComboBox#LoginModeSelector:focus,
QComboBox#ThemeModeSelector:focus,
QComboBox#MarketLoader:focus,
QComboBox#VanillaVersionCombo:focus,
QComboBox#ModpackInstanceCombo:focus {
    border: 2px solid @focus@;
}
QComboBox#LoginModeSelector:disabled,
QComboBox#ThemeModeSelector:disabled,
QComboBox#MarketLoader:disabled,
QComboBox#VanillaVersionCombo:disabled,
QComboBox#ModpackInstanceCombo:disabled {
    background: @disabledBg@;
    color: @disabledText@;
    border-color: @disabledBorder@;
}
QComboBox#LoginModeSelector::drop-down,
QComboBox#ThemeModeSelector::drop-down,
QComboBox#MarketLoader::drop-down,
QComboBox#VanillaVersionCombo::drop-down,
QComboBox#ModpackInstanceCombo::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 24px;
    border: none;
    background: transparent;
}
QComboBox#LoginModeSelector::down-arrow,
QComboBox#ThemeModeSelector::down-arrow,
QComboBox#MarketLoader::down-arrow,
QComboBox#VanillaVersionCombo::down-arrow,
QComboBox#ModpackInstanceCombo::down-arrow {
    image: none;
    width: 0;
    height: 0;
}
QComboBox#LoginModeSelector QAbstractItemView,
QComboBox#ThemeModeSelector QAbstractItemView,
QComboBox#MarketLoader QAbstractItemView,
QComboBox#VanillaVersionCombo QAbstractItemView,
QComboBox#ModpackInstanceCombo QAbstractItemView {
    background: @surface@;
    border: 2px solid @border@;
    color: @ink@;
    outline: none;
    selection-background-color: @primarySoft@;
    selection-color: @ink@;
}
QComboBox#MarketLoader {
    min-width: 132px;
    max-width: 148px;
}

/* ==================== 3D MINECRAFT BLOCK BUTTONS ==================== */
/* Stone Brick Secondary Button */
QPushButton#DialogAction,
QPushButton#AccountSignIn,
QPushButton#AccountSignOut,
QPushButton#MemoryPreset {
    min-height: 34px;
    padding: 0 14px;
    background: @surfaceAlt@;
    border-top: 2px solid @bevelHi@;
    border-left: 2px solid @bevelHi@;
    border-right: 2px solid @bevelLo@;
    border-bottom: 3px solid @bevelLo@;
    border-radius: 0px;
    color: @ink@;
    font-size: 12.5px;
    font-weight: 600;
}
QPushButton#DialogAction:hover,
QPushButton#AccountSignIn:hover,
QPushButton#AccountSignOut:hover,
QPushButton#MemoryPreset:hover {
    background: @surface@;
    border-top-color: @primaryHi@;
    border-left-color: @primaryHi@;
}
QPushButton#DialogAction:pressed,
QPushButton#AccountSignIn:pressed,
QPushButton#AccountSignOut:pressed,
QPushButton#MemoryPreset:pressed {
    background: @sunken@;
    border-top: 2px solid @bevelLo@;
    border-left: 2px solid @bevelLo@;
    border-bottom: 2px solid @bevelHi@;
    padding-top: 1px;
}
QPushButton#DialogAction:focus,
QPushButton#AccountSignIn:focus,
QPushButton#AccountSignOut:focus,
QPushButton#MemoryPreset:focus {
    border: 2px solid @focus@;
}
QPushButton#DialogAction:disabled,
QPushButton#AccountSignIn:disabled,
QPushButton#AccountSignOut:disabled,
QPushButton#MemoryPreset:disabled {
    background: @disabledBg@;
    border: 2px solid @disabledBorder@;
    color: @disabledText@;
}
QPushButton#MemoryPreset {
    min-width: 44px;
    padding: 0 8px;
    font-family: "Cascadia Mono", "Consolas", monospace;
}
QPushButton#MemoryPreset[active="true"] {
    background: @primarySoft@;
    border: 2px solid @primary@;
    color: @okText@;
    font-weight: 700;
}

/* Market Search Emerald/Lapis Action */
QPushButton#MarketSearch {
    min-height: 34px;
    min-width: 74px;
    padding: 0 16px;
    background: @primary@;
    border-top: 2px solid @primaryHi@;
    border-left: 2px solid @primaryHi@;
    border-right: 2px solid @primaryLo@;
    border-bottom: 3px solid @primaryLo@;
    border-radius: 0px;
    color: @onPrimary@;
    font-size: 13px;
    font-weight: 700;
}
QPushButton#MarketSearch:hover {
    background: @primaryHover@;
}
QPushButton#MarketSearch:pressed {
    background: @primaryLo@;
    padding-top: 1px;
}
QPushButton#MarketSearch:focus {
    border: 2px solid @focus@;
}
QPushButton#MarketSearch:disabled {
    background: @disabledBg@;
    border: 2px solid @disabledBorder@;
    color: @disabledText@;
}

/* Iconic Giant 3D Minecraft Emerald Launch CTA */
QPushButton#PrimaryAction {
    min-height: 48px;
    min-width: 158px;
    padding: 0 26px;
    background: @primary@;
    border-top: 3px solid @primaryHi@;
    border-left: 3px solid @primaryHi@;
    border-right: 3px solid @primaryLo@;
    border-bottom: 4px solid @primaryDeep@;
    border-radius: 0px;
    color: @onPrimary@;
    font-size: 15.5px;
    font-weight: 700;
}
QPushButton#PrimaryAction:hover {
    background: @primaryHover@;
}
QPushButton#PrimaryAction:pressed {
    background: @primaryLo@;
    border-top: 2px solid @primaryDeep@;
    border-left: 2px solid @primaryDeep@;
    border-bottom: 2px solid @primaryHi@;
    padding-top: 2px;
}
QPushButton#PrimaryAction:focus {
    border: 3px solid @gold@;
}
QPushButton#PrimaryAction:disabled {
    background: @disabledBg@;
    border: 2px solid @disabledBorder@;
    color: @disabledText@;
}

/* Text-style Secondary Action */
QPushButton#SecondaryAction {
    min-height: 30px;
    padding: 0 4px;
    border: 1px solid transparent;
    border-bottom: 2px solid @border@;
    border-radius: 0px;
    background: transparent;
    color: @inkSoft@;
    font-size: 12px;
    font-weight: 600;
}
QPushButton#SecondaryAction:hover {
    color: @ink@;
    border-bottom-color: @primary@;
}
QPushButton#SecondaryAction:focus {
    border: 2px solid @focus@;
}

/* South Gate Crimson Cancel / Danger Block */
QPushButton#CancelAction, QPushButton#AccountCancelSignIn {
    min-height: 40px;
    min-width: 120px;
    padding: 0 20px;
    background: @crimson@;
    border-top: 2px solid @crimsonHi@;
    border-left: 2px solid @crimsonHi@;
    border-right: 2px solid @crimsonLo@;
    border-bottom: 3px solid @crimsonLo@;
    border-radius: 0px;
    color: @onPrimary@;
    font-size: 12.5px;
    font-weight: 700;
}
QPushButton#CancelAction:hover, QPushButton#AccountCancelSignIn:hover {
    background: @crimsonHover@;
}
QPushButton#CancelAction:pressed, QPushButton#AccountCancelSignIn:pressed {
    background: @crimsonLo@;
    padding-top: 1px;
}
QPushButton#CancelAction:focus, QPushButton#AccountCancelSignIn:focus {
    border: 2px solid @gold@;
}
QPushButton#CancelAction:disabled, QPushButton#AccountCancelSignIn:disabled {
    background: @disabledBg@;
    border: 2px solid @disabledBorder@;
    color: @disabledText@;
}

/* ==================== MARKET LISTS & DETAIL ==================== */
QListWidget#MarketResults, QListWidget#MarketVersions {
    background: @sunken@;
    border-top: 2px solid @bevelLo@;
    border-left: 2px solid @bevelLo@;
    border-right: 2px solid @bevelHi@;
    border-bottom: 2px solid @bevelHi@;
    border-radius: 0px;
    outline: none;
    color: @ink@;
    font-size: 12.5px;
}
QListWidget#MarketResults::item, QListWidget#MarketVersions::item {
    padding: 7px 9px;
    border-bottom: 1px solid @lineSubtle@;
}
QListWidget#MarketResults::item:hover, QListWidget#MarketVersions::item:hover {
    background: @surfaceAlt@;
}
QListWidget#MarketResults::item:selected, QListWidget#MarketVersions::item:selected {
    background: @primarySoft@;
    color: @okText@;
    border-left: 3px solid @primary@;
}
QListWidget#MarketResults:focus, QListWidget#MarketVersions:focus {
    border: 2px solid @focus@;
}
QLabel#PagerLabel {
    color: @muted@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11.5px;
}
QLabel#MarketProjectTitle {
    color: @ink@;
    font-size: 19px;
    font-weight: 700;
}
QLabel#MarketProjectDescription {
    color: @inkSoft@;
    font-size: 12.5px;
}
QLabel#MarketDependency {
    color: @goldText@;
    background: @goldSoft@;
    border: 1px solid @goldLine@;
    padding: 6px 8px;
    font-size: 11.5px;
}
QLabel#MarketStatus {
    color: @inkSoft@;
    background: @sunken@;
    border: 2px solid @border@;
    border-radius: 0px;
    padding: 7px 9px;
    font-size: 11.5px;
}

/* ==================== CRAFTING SLOT DROPZONE & MODPACK ==================== */
QFrame#ImportDropZone {
    background: @dropBg@;
    border: 2px dashed @dropLine@;
    border-radius: 0px;
}
QLabel#ImportDropTitle {
    color: @okText@;
    font-size: 13px;
    font-weight: 700;
}
QLabel#ImportDropSub {
    color: @muted@;
    font-size: 11.5px;
}
QProgressBar#ImportProgress, QProgressBar#StageProgress {
    min-height: 12px;
    max-height: 12px;
    border: 2px solid @xpBorder@;
    border-radius: 0px;
    background: @xpTrack@;
    text-align: center;
}
QProgressBar#ImportProgress::chunk, QProgressBar#StageProgress::chunk {
    background: @primary@;
    border-radius: 0px;
}
QLabel#ModpackPackName {
    color: @ink@;
    font-size: 13px;
    font-weight: 700;
}
QLabel#ModpackPhase {
    color: @okText@;
    font-size: 12px;
    font-weight: 700;
}
QLabel#ModpackProgressText {
    color: @muted@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11.5px;
}
QLabel#ModpackError, QLabel#VanillaVersionsError {
    color: @crimsonHi@;
    background: @crimsonSoft@;
    border: 2px solid @crimsonLine@;
    border-radius: 0px;
    padding: 7px 9px;
    font-size: 12px;
}
QLabel#ModpackHint, QLabel#VanillaHint {
    color: @goldText@;
    background: @goldSoft@;
    border: 2px solid @goldLine@;
    border-radius: 0px;
    padding: 6px 8px;
    font-size: 11.5px;
}

/* ==================== INSTALL STAGES & BADGES ==================== */
QLabel#VersionLabel {
    color: @ink@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 24px;
    font-weight: 700;
}
QLabel#VersionMeta {
    color: @muted@;
    font-size: 12px;
}
QLabel#DetailLabel {
    color: @muted@;
    font-size: 11.5px;
    font-weight: 600;
}
QLabel#DetailNote {
    color: @muted@;
    font-size: 12px;
}
QLabel#BadgeRecommended {
    color: @goldText@;
    background: @goldSoft@;
    border: 2px solid @goldLine@;
    border-radius: 0px;
    padding: 0 8px;
    min-height: 20px;
    max-height: 20px;
    font-size: 10.5px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#BadgeVerified {
    color: @okText@;
    background: @primarySoft@;
    border: 2px solid @primary@;
    border-radius: 0px;
    padding: 0 8px;
    min-height: 20px;
    max-height: 20px;
    font-size: 10.5px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#StageDot {
    min-width: 26px;
    max-width: 26px;
    min-height: 26px;
    max-height: 26px;
    border-radius: 0px;
    background: @sunken@;
    border: 2px solid @border@;
    color: @muted@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#StageDot[stageState="done"] {
    background: @primary@;
    border-top: 2px solid @primaryHi@;
    border-left: 2px solid @primaryHi@;
    border-right: 2px solid @primaryLo@;
    border-bottom: 2px solid @primaryLo@;
    color: @onPrimary@;
}
QLabel#StageDot[stageState="current"] {
    background: @gold@;
    border: 2px solid @goldLine@;
    color: #1A1200;
    font-weight: 700;
}
QLabel#StageTitle {
    color: @ink@;
    font-size: 13px;
    font-weight: 700;
}
QLabel#StageTitle[stageState="future"] {
    color: @muted@;
    font-weight: 400;
}
QLabel#StageSub {
    color: @muted@;
    font-size: 11.5px;
}
QLabel#StageSub[stageState="current"] {
    color: @okText@;
}
QFrame#StageLine {
    background: @border@;
    border: none;
    max-width: 2px;
}
QLabel#ProgressText {
    color: @muted@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11px;
}
QFrame#UnknownNote {
    background: @dropBg@;
    border: 2px dashed @dropLine@;
    border-radius: 0px;
}
QLabel#UnknownTitle {
    color: @okText@;
    font-size: 11.5px;
    font-weight: 700;
}
QLabel#UnknownValue {
    color: @inkSoft@;
    font-family: "Cascadia Mono", "Consolas", monospace;
    font-size: 11.5px;
}

/* ==================== DOCK ACTION BAR & ERRORS ==================== */
QFrame#ActionBar {
    background: @surfaceAlt@;
    border: none;
    border-top: 2px solid @border@;
    border-radius: 0px;
}
QFrame#ActionBar[error="true"] {
    background: @crimsonSoft@;
    border-top: 2px solid @crimsonLine@;
}
QLabel#ActionTitle {
    color: @ink@;
    font-size: 14.5px;
    font-weight: 700;
}
QLabel#ActionTitle[error="true"] {
    color: @crimsonHi@;
}
QLabel#ActionNote {
    color: @muted@;
    font-size: 12px;
}
QFrame#ErrorNotice {
    background: @crimsonSoft@;
    border: 2px solid @crimsonLine@;
    border-left: 4px solid @crimson@;
    border-radius: 0px;
}
QLabel#ErrorTitle {
    color: @crimsonHi@;
    font-size: 12.5px;
    font-weight: 700;
}
QLabel#ErrorDetail {
    color: @inkSoft@;
    font-size: 12px;
}

/* ==================== TOOLTIPS & SCROLLBARS ==================== */
QToolTip {
    color: @railStrong@;
    background: @rail@;
    border: 2px solid @primary@;
    padding: 5px 8px;
}
QScrollBar:vertical {
    background: @sunken@;
    width: 10px;
    margin: 0;
    border-left: 1px solid @border@;
}
QScrollBar::handle:vertical {
    background: @border@;
    min-height: 28px;
    border-radius: 0px;
}
QScrollBar::handle:vertical:hover {
    background: @primary@;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
)QSS");

    for (const Token& token : tokens) {
        qss.replace(QStringLiteral("@%1@").arg(QLatin1String(token.name)),
                    QLatin1String(token.value));
    }
    return qss;
}

} // namespace hustmcl::ui::theme
