#include "theme.h"

namespace hustmcl::ui::theme {

QFont baseFont()
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSizeF(10.0);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

namespace {
struct Token { const char* name; const char* value; };
// QSS below is written with @name@ placeholders so colours live only in theme.h.
constexpr Token kTokens[] = {
    {"page", kPage}, {"surface", kSurface}, {"sunken", kSunken}, {"ink", kInk}, {"inkSoft", kInkSoft},
    {"inkHover", kInkHover}, {"muted", kMuted}, {"line", kLine}, {"lineSubtle", kLineSubtle},
    {"lineFaint", kLineFaint}, {"stageLine", kStageLine}, {"linkLine", kLinkLine},
    {"primary", kPrimary}, {"primaryHover", kPrimaryHover}, {"primaryDeep", kPrimaryDeep},
    {"primarySoft", kPrimarySoft}, {"primaryTint", kPrimaryTint}, {"ok", kOk}, {"okSoft", kOkSoft},
    {"moss", kMoss}, {"caption", kCaption}, {"dropBg", kDropBg}, {"dropLine", kDropLine},
    {"unknownText", kUnknownText}, {"warn", kWarn}, {"warnSoft", kWarnSoft}, {"warnLine", kWarnLine},
    {"error", kError}, {"errorSoft", kErrorSoft}, {"errorLine", kErrorLine}, {"cancel", kCancel},
    {"cancelLine", kCancelLine}, {"cancelTint", kCancelTint}, {"focus", kFocus},
    {"disabledText", kDisabledText}, {"disabledBg", kDisabledBg}, {"onPrimary", kOnPrimary},
    {"gold", kGold}, {"goldSoft", kGoldSoft}, {"goldText", kGoldText}, {"rail", kRail},
    {"railHover", kRailHover}, {"railSelected", kRailSelected}, {"railStrong", kRailStrong},
    {"railText", kRailText}, {"railMuted", kRailMuted}, {"railFoot", kRailFoot}, {"railOff", kRailOff},
};
} // namespace
QString styleSheet()
{
    QString qss = QStringLiteral(R"QSS(
QWidget {
    color: @ink@;
}
QWidget#Root {
    background: @page@;
}
QWidget#Sidebar {
    background: @rail@;
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
    font-size: 17px;
    font-weight: 700;
}
QLabel#BrandSub {
    color: @railMuted@;
    font-size: 10.5px;
}
QLabel#DemoBadge {
    color: @gold@;
    background: @railHover@;
    border: 1px solid @railSelected@;
    border-radius: 10px;
    padding: 4px 9px;
    font-size: 10.5px;
}
QLabel#DemoNote {
    color: @railMuted@;
    font-size: 10.5px;
}
QLabel#NavNote {
    color: @railMuted@;
    font-size: 11px;
}
QLabel#SidebarCaption {
    color: @caption@;
    font-size: 12.5px;
    font-weight: 600;
}
QLabel#SidebarSub {
    color: @muted@;
    font-size: 11px;
}
QWidget#Sidebar QLabel#SidebarCaption {
    color: @railStrong@;
}
QWidget#Sidebar QLabel#SidebarSub {
    color: @railMuted@;
}
QLabel#AccountName {
    color: @railStrong@;
    font-size: 13px;
    font-weight: 600;
}
QLabel#AccountState {
    color: @railMuted@;
    font-size: 11px;
}
QLabel#Avatar {
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    border-radius: 16px;
    background: @railSelected@;
    color: @railStrong@;
    font-size: 13px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#RailFoot {
    color: @railFoot@;
    font-size: 10.5px;
}
QFrame#Divider {
    background: @line@;
    border: none;
    max-height: 1px;
}
QWidget#Sidebar QFrame#Divider {
    background: @railSelected@;
}
QFrame#VDivider {
    background: @line@;
    border: none;
    max-width: 1px;
}
QWidget#Sidebar QPushButton#SecondaryAction {
    color: @railText@;
    border-bottom: 1px solid @railOff@;
}
QWidget#Sidebar QPushButton#SecondaryAction:hover {
    color: @railStrong@;
}
QWidget#Sidebar QPushButton#SecondaryAction:focus {
    border: 1px solid @gold@;
}
QListWidget#Navigation {
    background: transparent;
    border: none;
    outline: none;
    font-size: 14px;
    color: @railText@;
}
QListWidget#Navigation::item {
    height: 44px;
    padding-left: 14px;
    border-radius: 6px;
    border-left: 3px solid transparent;
}
QListWidget#Navigation::item:hover {
    background: @railHover@;
    color: @railStrong@;
}
QListWidget#Navigation::item:selected {
    background: @railSelected@;
    color: @railStrong@;
    border-left: 3px solid @gold@;
    font-weight: 700;
}
QListWidget#Navigation::item:disabled {
    color: @railOff@;
}
QListWidget#Navigation:focus {
    border: 1px solid @gold@;
    border-radius: 6px;
}
QLabel#PageTitle {
    color: @ink@;
    font-size: 30px;
    font-weight: 600;
}
QLabel#PageSubtitle {
    color: @muted@;
    font-size: 13px;
}
QLabel#HeaderMeta {
    color: @muted@;
    font-size: 12px;
}
QFrame#Desk {
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 14px;
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
QLabel#InstanceName {
    color: @ink@;
    font-size: 30px;
    font-weight: 600;
}
QLabel#InstanceMeta {
    color: @muted@;
    font-size: 13px;
}
QLabel#SectionTitle {
    color: @ink@;
    font-size: 12px;
    font-weight: 600;
}
QLabel#StatusCheck {
    min-width: 18px;
    max-width: 18px;
    min-height: 18px;
    max-height: 18px;
    border-radius: 9px;
    background: @okSoft@;
    color: @ok@;
    font-size: 11px;
    qproperty-alignment: AlignCenter;
}
QLabel#StatusCheck[ok="false"] {
    background: @warnSoft@;
    color: @warn@;
}
QLabel#StatusName {
    color: @ink@;
    font-size: 13px;
    font-weight: 600;
}
QLabel#StatusValue {
    color: @muted@;
    font-size: 12.5px;
}
QLabel#StatusState {
    color: @moss@;
    font-size: 11.5px;
}
QLabel#StatusState[ok="false"] {
    color: @warn@;
}
QLabel#SummaryLabel {
    color: @ink@;
    font-size: 12.5px;
}
QLabel#SummaryValue {
    color: @ink@;
    font-size: 12.5px;
    font-weight: 600;
}
QLabel#Footnote {
    color: @muted@;
    font-size: 11.5px;
}
QLabel#InstallPath, QLabel#PathValue {
    color: @inkSoft@;
    background: @sunken@;
    border: 1px solid @line@;
    border-radius: 6px;
    padding: 7px 10px;
    font-size: 11px;
}
QLabel#LogStatusTitle {
    color: @ink@;
    font-size: 18px;
    font-weight: 600;
}
QLabel#CheckDetail {
    color: @muted@;
    font-size: 12px;
}
QLabel#FieldError {
    color: @error@;
    font-size: 12px;
    padding-top: 4px;
}
QLabel#FieldSuccess {
    color: @ok@;
    font-size: 12px;
    padding-top: 4px;
}
QLabel#SettingsAccountName {
    color: @ink@;
    font-size: 16px;
    font-weight: 600;
}
QLabel#SettingsAccountState {
    color: @moss@;
    font-size: 12px;
}
QLabel#AccountMessage {
    color: @muted@;
    background: @sunken@;
    border: 1px solid @line@;
    border-radius: 6px;
    padding: 6px 9px;
    font-size: 12px;
}
QLabel#AccountMessage[error="true"] {
    color: @error@;
    background: @errorSoft@;
    border: 1px solid @errorLine@;
}
QLineEdit#PlayerNameInput {
    min-height: 36px;
    padding: 0 10px;
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 7px;
    color: @ink@;
    font-size: 13px;
    selection-background-color: @primarySoft@;
    selection-color: @ink@;
}
QLineEdit#PlayerNameInput:focus {
    border: 2px solid @focus@;
    padding: 0 9px;
}
QLineEdit#PlayerNameInput:disabled {
    background: @sunken@;
    color: @disabledText@;
    border-color: @line@;
}
QPushButton#DialogAction, QPushButton#AccountSignIn, QPushButton#AccountSignOut {
    min-height: 36px;
    padding: 0 16px;
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 7px;
    color: @ink@;
    font-size: 12.5px;
}
QPushButton#DialogAction:hover, QPushButton#AccountSignIn:hover, QPushButton#AccountSignOut:hover {
    background: @sunken@;
}
QPushButton#DialogAction:focus, QPushButton#AccountSignIn:focus, QPushButton#AccountSignOut:focus {
    border: 2px solid @focus@;
    padding: 0 15px;
}
QPushButton#DialogAction:disabled, QPushButton#AccountSignIn:disabled, QPushButton#AccountSignOut:disabled {
    background: @sunken@;
    border-color: @lineFaint@;
    color: @disabledText@;
}
QLineEdit#MarketSearchInput, QLineEdit#MarketVersionInput {
    min-height: 36px;
    padding: 0 10px;
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 7px;
    color: @ink@;
    font-size: 12.5px;
    selection-background-color: @primarySoft@;
    selection-color: @ink@;
}
QLineEdit#MarketVersionInput {
    min-width: 150px;
    max-width: 150px;
}
QLineEdit#MarketSearchInput:focus, QLineEdit#MarketVersionInput:focus {
    border: 2px solid @focus@;
    padding: 0 9px;
}
QLineEdit#MarketSearchInput:disabled, QLineEdit#MarketVersionInput:disabled {
    background: @sunken@;
    color: @disabledText@;
    border-color: @line@;
}
QComboBox#MarketLoader {
    min-height: 36px;
    min-width: 132px;
    max-width: 146px;
    padding: 0 8px;
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 7px;
    color: @ink@;
    font-size: 12.5px;
}
QComboBox#MarketLoader:focus {
    border: 2px solid @focus@;
    padding: 0 7px;
}
QComboBox#MarketLoader:disabled {
    background: @sunken@;
    color: @disabledText@;
}
QPushButton#MarketSearch {
    min-height: 36px;
    min-width: 70px;
    padding: 0 16px;
    background: @ink@;
    border: 1px solid @ink@;
    border-radius: 7px;
    color: @surface@;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#MarketSearch:hover {
    background: @inkHover@;
}
QPushButton#MarketSearch:focus {
    border: 2px solid @focus@;
    padding: 0 15px;
}
QPushButton#MarketSearch:disabled {
    background: @disabledBg@;
    border-color: @disabledBg@;
    color: @disabledText@;
}
QListWidget#MarketResults, QListWidget#MarketVersions {
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 7px;
    outline: none;
    color: @ink@;
    font-size: 12.5px;
}
QListWidget#MarketResults::item, QListWidget#MarketVersions::item {
    padding: 6px 9px;
    border-bottom: 1px solid @lineSubtle@;
}
QListWidget#MarketResults::item:hover, QListWidget#MarketVersions::item:hover {
    background: @sunken@;
}
QListWidget#MarketResults::item:selected, QListWidget#MarketVersions::item:selected {
    background: @primarySoft@;
    color: @primary@;
}
QListWidget#MarketResults:focus, QListWidget#MarketVersions:focus {
    border: 2px solid @focus@;
}
QLabel#PagerLabel {
    color: @muted@;
    font-size: 11.5px;
}
QLabel#MarketProjectTitle {
    color: @ink@;
    font-size: 20px;
    font-weight: 600;
}
QLabel#MarketProjectDescription {
    color: @inkSoft@;
    font-size: 13px;
}
QLabel#MarketDependency {
    color: @muted@;
    font-size: 11.5px;
    padding-top: 5px;
}
QLabel#MarketStatus {
    color: @inkSoft@;
    background: @sunken@;
    border: 1px solid @line@;
    border-radius: 6px;
    padding: 7px 10px;
    font-size: 11.5px;
}
QFrame#ImportDropZone {
    background: @dropBg@;
    border: 1px dashed @dropLine@;
    border-radius: 8px;
}
QLabel#ImportDropTitle {
    color: @primary@;
    font-size: 13px;
    font-weight: 600;
}
QLabel#ImportDropSub {
    color: @muted@;
    font-size: 11.5px;
}
QComboBox#ModpackInstanceCombo {
    min-height: 34px;
    padding: 0 8px;
    background: @surface@;
    border: 1px solid @line@;
    border-radius: 7px;
    color: @ink@;
    font-size: 12.5px;
}
QComboBox#ModpackInstanceCombo:focus {
    border: 2px solid @focus@;
    padding: 0 7px;
}
QComboBox#ModpackInstanceCombo:disabled {
    background: @sunken@;
    color: @disabledText@;
    border-color: @lineFaint@;
}
QProgressBar#ImportProgress {
    min-height: 8px;
    max-height: 8px;
    border: none;
    border-radius: 4px;
    background: @lineSubtle@;
    text-align: center;
}
QProgressBar#ImportProgress::chunk {
    background: @primary@;
    border-radius: 4px;
}
QLabel#ModpackPackName {
    color: @ink@;
    font-size: 13px;
    font-weight: 600;
}
QLabel#ModpackPhase {
    color: @primary@;
    font-size: 12px;
    font-weight: 600;
}
QLabel#ModpackProgressText {
    color: @muted@;
    font-size: 11.5px;
}
QLabel#ModpackError {
    color: @error@;
    background: @errorSoft@;
    border: 1px solid @errorLine@;
    border-radius: 7px;
    padding: 7px 9px;
    font-size: 12px;
}
QLabel#ModpackHint {
    color: @warn@;
    background: @warnSoft@;
    border: 1px solid @warnLine@;
    border-radius: 6px;
    padding: 6px 8px;
    font-size: 11.5px;
}
QLabel#VersionLabel {
    color: @ink@;
    font-size: 26px;
    font-weight: 600;
}
QLabel#VersionMeta {
    color: @muted@;
    font-size: 12px;
}
QLabel#DetailLabel {
    color: @muted@;
    font-size: 11.5px;
}
QLabel#DetailNote {
    color: @muted@;
    font-size: 13px;
}
QLabel#BadgeRecommended {
    color: @goldText@;
    background: @goldSoft@;
    border-radius: 10px;
    padding: 0 9px;
    min-height: 20px;
    max-height: 20px;
    font-size: 10px;
    qproperty-alignment: AlignCenter;
}
QLabel#BadgeVerified {
    color: @ok@;
    background: @okSoft@;
    border-radius: 10px;
    padding: 0 9px;
    min-height: 20px;
    max-height: 20px;
    font-size: 10px;
    qproperty-alignment: AlignCenter;
}
QLabel#StageDot {
    min-width: 27px;
    max-width: 27px;
    min-height: 27px;
    max-height: 27px;
    border-radius: 13px;
    background: @surface@;
    border: 1px solid @line@;
    color: @muted@;
    font-size: 11px;
    qproperty-alignment: AlignCenter;
}
QLabel#StageDot[stageState="done"] {
    background: @primary@;
    border: 1px solid @primary@;
    color: @onPrimary@;
}
QLabel#StageDot[stageState="current"] {
    background: @primarySoft@;
    border: 2px solid @primary@;
    color: @primary@;
    font-weight: 700;
}
QLabel#StageTitle {
    color: @ink@;
    font-size: 13.5px;
    font-weight: 600;
}
QLabel#StageTitle[stageState="future"] {
    color: @muted@;
    font-weight: 400;
}
QLabel#StageSub {
    color: @muted@;
    font-size: 12px;
}
QLabel#StageSub[stageState="current"] {
    color: @primaryDeep@;
}
QFrame#StageLine {
    background: @stageLine@;
    border: none;
    max-width: 1px;
}
QProgressBar#StageProgress {
    min-height: 8px;
    max-height: 8px;
    border: none;
    border-radius: 4px;
    background: @lineSubtle@;
    text-align: center;
}
QProgressBar#StageProgress::chunk {
    background: @primary@;
    border-radius: 4px;
}
QLabel#ProgressText {
    color: @muted@;
    font-size: 11.5px;
}
QFrame#UnknownNote {
    background: @dropBg@;
    border: 1px dashed @dropLine@;
    border-radius: 6px;
}
QLabel#UnknownTitle {
    color: @primary@;
    font-size: 11.5px;
    font-weight: 600;
}
QLabel#UnknownValue {
    color: @unknownText@;
    font-size: 11.5px;
}
QFrame#ActionBar {
    background: @surface@;
    border: none;
    border-top: 1px solid @line@;
    border-bottom-left-radius: 14px;
    border-bottom-right-radius: 14px;
}
QFrame#ActionBar[error="true"] {
    background: @errorSoft@;
    border-top: 1px solid @errorLine@;
}
QLabel#ActionTitle {
    color: @ink@;
    font-size: 15px;
    font-weight: 600;
}
QLabel#ActionTitle[error="true"] {
    color: @error@;
}
QLabel#ActionNote {
    color: @muted@;
    font-size: 12px;
}
QFrame#ErrorNotice {
    background: @errorSoft@;
    border: 1px solid @errorLine@;
    border-radius: 7px;
}
QLabel#ErrorTitle {
    color: @error@;
    font-size: 12px;
    font-weight: 600;
}
QLabel#ErrorDetail {
    color: @muted@;
    font-size: 12px;
}
QPushButton#PrimaryAction {
    min-height: 52px;
    min-width: 152px;
    padding: 0 24px;
    border: 2px solid @primaryDeep@;
    border-bottom: 4px solid @primaryDeep@;
    border-radius: 8px;
    background: @primary@;
    color: @onPrimary@;
    font-size: 16px;
    font-weight: 700;
}
QPushButton#PrimaryAction:hover {
    background: @primaryHover@;
}
QPushButton#PrimaryAction:pressed {
    background: @primaryDeep@;
    border-bottom: 2px solid @primaryDeep@;
    padding-top: 2px;
}
QPushButton#PrimaryAction:focus {
    border: 2px solid @focus@;
    border-bottom: 4px solid @focus@;
}
QPushButton#PrimaryAction:disabled {
    background: @disabledBg@;
    border-color: @line@;
    color: @disabledText@;
}
QPushButton#SecondaryAction {
    min-height: 32px;
    padding: 0 2px;
    border: 1px solid transparent;
    border-bottom: 1px solid @linkLine@;
    background: transparent;
    color: @muted@;
    font-size: 12.5px;
}
QPushButton#SecondaryAction:hover {
    color: @ink@;
}
QPushButton#SecondaryAction:focus {
    border: 1px solid @focus@;
    border-bottom: 1px solid @focus@;
}
QPushButton#CancelAction, QPushButton#AccountCancelSignIn {
    min-height: 44px;
    min-width: 128px;
    padding: 0 22px;
    border: 2px solid @cancelLine@;
    border-radius: 7px;
    background: @surface@;
    color: @cancel@;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#CancelAction:hover, QPushButton#AccountCancelSignIn:hover {
    background: @cancelTint@;
}
QPushButton#CancelAction:focus, QPushButton#AccountCancelSignIn:focus {
    border: 2px solid @focus@;
}
QPushButton#CancelAction:disabled, QPushButton#AccountCancelSignIn:disabled {
    border-color: @line@;
    color: @disabledText@;
    background: @sunken@;
}
QToolTip {
    color: @surface@;
    background: @ink@;
    border: none;
    padding: 6px 8px;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 4px 0;
}
QScrollBar::handle:vertical {
    background: @line@;
    min-height: 30px;
    border-radius: 5px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
)QSS");
    for (const Token& token : kTokens) {
        qss.replace(QStringLiteral("@%1@").arg(QLatin1String(token.name)), QLatin1String(token.value));
    }
    return qss;
}

} // namespace hustmcl::ui::theme
