#include "theme.h"

namespace hustmcl::ui::theme {

QFont baseFont()
{
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPointSizeF(10.0);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QString styleSheet()
{
    return QStringLiteral(R"QSS(
QWidget {
    color: #23201C;
}
QWidget#Root {
    background: #F4F0E7;
}
QWidget#Sidebar {
    background: #FBF8F1;
    border-right: 1px solid #DED5C6;
}
QLabel#BrandMark {
    min-width: 30px;
    max-width: 30px;
    min-height: 30px;
    max-height: 30px;
    border-radius: 8px;
    background: #8C1D2F;
    color: #FFFFFF;
    font-size: 15px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#BrandName {
    font-size: 17px;
    font-weight: 700;
}
QLabel#BrandSub {
    color: #6F675D;
    font-size: 10.5px;
}
QLabel#DemoBadge {
    color: #8C1D2F;
    background: #F2E3DF;
    border-radius: 10px;
    padding: 4px 9px;
    font-size: 10.5px;
}
QLabel#DemoNote {
    color: #6F675D;
    font-size: 10.5px;
}
QLabel#NavNote {
    color: #6F675D;
    font-size: 11px;
}
QLabel#SidebarCaption {
    color: #4C647B;
    font-size: 12.5px;
    font-weight: 600;
}
QLabel#SidebarSub {
    color: #6F675D;
    font-size: 11px;
}
QLabel#AccountName {
    color: #23201C;
    font-size: 13px;
    font-weight: 600;
}
QLabel#AccountState {
    color: #6F675D;
    font-size: 11px;
}
QLabel#Avatar {
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
    border-radius: 16px;
    background: #F2E3DF;
    color: #8C1D2F;
    font-size: 13px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}
QLabel#RailFoot {
    color: #6F675D;
    font-size: 10.5px;
}
QFrame#Divider {
    background: #DED5C6;
    border: none;
    max-height: 1px;
}
QFrame#VDivider {
    background: #DED5C6;
    border: none;
    max-width: 1px;
}
QListWidget#Navigation {
    background: transparent;
    border: none;
    outline: none;
    font-size: 14px;
    color: #6F675D;
}
QListWidget#Navigation::item {
    height: 46px;
    padding-left: 12px;
    border-radius: 7px;
    border-left: 3px solid transparent;
}
QListWidget#Navigation::item:hover {
    background: #F2ECE2;
    color: #23201C;
}
QListWidget#Navigation::item:selected {
    background: #F2E3DF;
    color: #8C1D2F;
    border-left: 3px solid #8C1D2F;
    font-weight: 700;
}
QListWidget#Navigation::item:disabled {
    color: #A8A094;
}
QListWidget#Navigation:focus {
    border: 1px solid #1D5FA7;
}
QLabel#PageTitle {
    color: #23201C;
    font-size: 30px;
    font-weight: 600;
}
QLabel#PageSubtitle {
    color: #6F675D;
    font-size: 13px;
}
QLabel#HeaderMeta {
    color: #6F675D;
    font-size: 12px;
}
QFrame#Desk {
    background: #FFFCF6;
    border: 1px solid #DED5C6;
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
    color: #23201C;
    font-size: 30px;
    font-weight: 600;
}
QLabel#InstanceMeta {
    color: #6F675D;
    font-size: 13px;
}
QLabel#SectionTitle {
    color: #23201C;
    font-size: 12px;
    font-weight: 600;
}
QLabel#StatusCheck {
    min-width: 18px;
    max-width: 18px;
    min-height: 18px;
    max-height: 18px;
    border-radius: 9px;
    background: #E8F1EC;
    color: #2F6B4F;
    font-size: 11px;
    qproperty-alignment: AlignCenter;
}
QLabel#StatusCheck[ok="false"] {
    background: #FBF1DC;
    color: #8A5A00;
}
QLabel#StatusName {
    color: #23201C;
    font-size: 13px;
    font-weight: 600;
}
QLabel#StatusValue {
    color: #6F675D;
    font-size: 12.5px;
}
QLabel#StatusState {
    color: #4F6B52;
    font-size: 11.5px;
}
QLabel#StatusState[ok="false"] {
    color: #8A5A00;
}
QLabel#SummaryLabel {
    color: #23201C;
    font-size: 12.5px;
}
QLabel#SummaryValue {
    color: #23201C;
    font-size: 12.5px;
    font-weight: 600;
}
QLabel#Footnote {
    color: #6F675D;
    font-size: 11.5px;
}
QLabel#InstallPath, QLabel#PathValue {
    color: #4D463D;
    background: #F8F4EC;
    border: 1px solid #DED5C6;
    border-radius: 6px;
    padding: 7px 10px;
    font-size: 11px;
}
QLabel#LogStatusTitle {
    color: #23201C;
    font-size: 18px;
    font-weight: 600;
}
QLabel#CheckDetail {
    color: #6F675D;
    font-size: 12px;
}
QLabel#FieldError {
    color: #B3261E;
    font-size: 12px;
    padding-top: 4px;
}
QLabel#FieldSuccess {
    color: #2F6B4F;
    font-size: 12px;
    padding-top: 4px;
}
QLineEdit#PlayerNameInput {
    min-height: 36px;
    padding: 0 10px;
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    border-radius: 7px;
    color: #23201C;
    font-size: 13px;
    selection-background-color: #F2E3DF;
    selection-color: #23201C;
}
QLineEdit#PlayerNameInput:focus {
    border: 2px solid #1D5FA7;
    padding: 0 9px;
}
QLineEdit#PlayerNameInput:disabled {
    background: #F8F4EC;
    color: #7A838D;
    border-color: #DED5C6;
}
QPushButton#DialogAction {
    min-height: 36px;
    padding: 0 16px;
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    border-radius: 7px;
    color: #23201C;
    font-size: 12.5px;
}
QPushButton#DialogAction:hover {
    background: #F8F4EC;
}
QPushButton#DialogAction:focus {
    border: 2px solid #1D5FA7;
    padding: 0 15px;
}
QPushButton#DialogAction:disabled {
    background: #F8F4EC;
    border-color: #EDE6DA;
    color: #7A838D;
}
QLineEdit#MarketSearchInput, QLineEdit#MarketVersionInput {
    min-height: 36px;
    padding: 0 10px;
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    border-radius: 7px;
    color: #23201C;
    font-size: 12.5px;
    selection-background-color: #F2E3DF;
    selection-color: #23201C;
}
QLineEdit#MarketVersionInput {
    min-width: 150px;
    max-width: 150px;
}
QLineEdit#MarketSearchInput:focus, QLineEdit#MarketVersionInput:focus {
    border: 2px solid #1D5FA7;
    padding: 0 9px;
}
QLineEdit#MarketSearchInput:disabled, QLineEdit#MarketVersionInput:disabled {
    background: #F8F4EC;
    color: #7A838D;
    border-color: #DED5C6;
}
QComboBox#MarketLoader {
    min-height: 36px;
    min-width: 132px;
    max-width: 146px;
    padding: 0 8px;
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    border-radius: 7px;
    color: #23201C;
    font-size: 12.5px;
}
QComboBox#MarketLoader:focus {
    border: 2px solid #1D5FA7;
    padding: 0 7px;
}
QComboBox#MarketLoader:disabled {
    background: #F8F4EC;
    color: #7A838D;
}
QPushButton#MarketSearch {
    min-height: 36px;
    min-width: 70px;
    padding: 0 16px;
    background: #23201C;
    border: 1px solid #23201C;
    border-radius: 7px;
    color: #FFFCF6;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#MarketSearch:hover {
    background: #3A342D;
}
QPushButton#MarketSearch:focus {
    border: 2px solid #1D5FA7;
    padding: 0 15px;
}
QPushButton#MarketSearch:disabled {
    background: #E7EBEF;
    border-color: #E7EBEF;
    color: #7A838D;
}
QListWidget#MarketResults, QListWidget#MarketVersions {
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    border-radius: 7px;
    outline: none;
    color: #23201C;
    font-size: 12.5px;
}
QListWidget#MarketResults::item, QListWidget#MarketVersions::item {
    padding: 6px 9px;
    border-bottom: 1px solid #F0E9DD;
}
QListWidget#MarketResults::item:hover, QListWidget#MarketVersions::item:hover {
    background: #F8F4EC;
}
QListWidget#MarketResults::item:selected, QListWidget#MarketVersions::item:selected {
    background: #F2E3DF;
    color: #8C1D2F;
}
QListWidget#MarketResults:focus, QListWidget#MarketVersions:focus {
    border: 2px solid #1D5FA7;
}
QLabel#PagerLabel {
    color: #6F675D;
    font-size: 11.5px;
}
QLabel#MarketProjectTitle {
    color: #23201C;
    font-size: 20px;
    font-weight: 600;
}
QLabel#MarketProjectDescription {
    color: #4D463D;
    font-size: 13px;
}
QLabel#MarketDependency {
    color: #6F675D;
    font-size: 11.5px;
    padding-top: 5px;
}
QLabel#MarketStatus {
    color: #4D463D;
    background: #F8F4EC;
    border: 1px solid #DED5C6;
    border-radius: 6px;
    padding: 7px 10px;
    font-size: 11.5px;
}
QFrame#ImportDropZone {
    background: #FDF7F4;
    border: 1px dashed #D8B9B2;
    border-radius: 8px;
}
QLabel#ImportDropTitle {
    color: #8C1D2F;
    font-size: 13px;
    font-weight: 600;
}
QLabel#ImportDropSub {
    color: #6F675D;
    font-size: 11.5px;
}
QComboBox#ModpackInstanceCombo {
    min-height: 34px;
    padding: 0 8px;
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    border-radius: 7px;
    color: #23201C;
    font-size: 12.5px;
}
QComboBox#ModpackInstanceCombo:focus {
    border: 2px solid #1D5FA7;
    padding: 0 7px;
}
QComboBox#ModpackInstanceCombo:disabled {
    background: #F8F4EC;
    color: #7A838D;
    border-color: #EDE6DA;
}
QProgressBar#ImportProgress {
    min-height: 8px;
    max-height: 8px;
    border: none;
    border-radius: 4px;
    background: #F0E9DD;
    text-align: center;
}
QProgressBar#ImportProgress::chunk {
    background: #8C1D2F;
    border-radius: 4px;
}
QLabel#ModpackPackName {
    color: #23201C;
    font-size: 13px;
    font-weight: 600;
}
QLabel#ModpackPhase {
    color: #8C1D2F;
    font-size: 12px;
    font-weight: 600;
}
QLabel#ModpackProgressText {
    color: #6F675D;
    font-size: 11.5px;
}
QLabel#ModpackError {
    color: #B3261E;
    background: #FBEAE9;
    border: 1px solid #E3B4B0;
    border-radius: 7px;
    padding: 7px 9px;
    font-size: 12px;
}
QLabel#ModpackHint {
    color: #8A5A00;
    background: #FBF1DC;
    border: 1px solid #E7D3A8;
    border-radius: 6px;
    padding: 6px 8px;
    font-size: 11.5px;
}
QLabel#VersionLabel {
    color: #23201C;
    font-size: 26px;
    font-weight: 600;
}
QLabel#VersionMeta {
    color: #6F675D;
    font-size: 12px;
}
QLabel#DetailLabel {
    color: #6F675D;
    font-size: 11.5px;
}
QLabel#DetailNote {
    color: #6F675D;
    font-size: 13px;
}
QLabel#BadgeRecommended {
    color: #8C1D2F;
    background: #F2E3DF;
    border-radius: 10px;
    padding: 0 9px;
    min-height: 20px;
    max-height: 20px;
    font-size: 10px;
    qproperty-alignment: AlignCenter;
}
QLabel#BadgeVerified {
    color: #2F6B4F;
    background: #E8F1EC;
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
    background: #FFFCF6;
    border: 1px solid #DED5C6;
    color: #6F675D;
    font-size: 11px;
    qproperty-alignment: AlignCenter;
}
QLabel#StageDot[stageState="done"] {
    background: #8C1D2F;
    border: 1px solid #8C1D2F;
    color: #FFFFFF;
}
QLabel#StageDot[stageState="current"] {
    background: #F2E3DF;
    border: 2px solid #8C1D2F;
    color: #8C1D2F;
    font-weight: 700;
}
QLabel#StageTitle {
    color: #23201C;
    font-size: 13.5px;
    font-weight: 600;
}
QLabel#StageTitle[stageState="future"] {
    color: #6F675D;
    font-weight: 400;
}
QLabel#StageSub {
    color: #6F675D;
    font-size: 12px;
}
QLabel#StageSub[stageState="current"] {
    color: #6E1524;
}
QFrame#StageLine {
    background: #E8DFD1;
    border: none;
    max-width: 1px;
}
QProgressBar#StageProgress {
    min-height: 8px;
    max-height: 8px;
    border: none;
    border-radius: 4px;
    background: #F0E9DD;
    text-align: center;
}
QProgressBar#StageProgress::chunk {
    background: #8C1D2F;
    border-radius: 4px;
}
QLabel#ProgressText {
    color: #6F675D;
    font-size: 11.5px;
}
QFrame#UnknownNote {
    background: #FDF7F4;
    border: 1px dashed #D8B9B2;
    border-radius: 6px;
}
QLabel#UnknownTitle {
    color: #8C1D2F;
    font-size: 11.5px;
    font-weight: 600;
}
QLabel#UnknownValue {
    color: #5F4A43;
    font-size: 11.5px;
}
QFrame#ActionBar {
    background: #FFFCF6;
    border: none;
    border-top: 1px solid #DED5C6;
    border-bottom-left-radius: 14px;
    border-bottom-right-radius: 14px;
}
QFrame#ActionBar[error="true"] {
    background: #FBEAE9;
    border-top: 1px solid #E3B4B0;
}
QLabel#ActionTitle {
    color: #23201C;
    font-size: 15px;
    font-weight: 600;
}
QLabel#ActionTitle[error="true"] {
    color: #B3261E;
}
QLabel#ActionNote {
    color: #6F675D;
    font-size: 12px;
}
QFrame#ErrorNotice {
    background: #FBEAE9;
    border: 1px solid #E3B4B0;
    border-radius: 7px;
}
QLabel#ErrorTitle {
    color: #B3261E;
    font-size: 12px;
    font-weight: 600;
}
QLabel#ErrorDetail {
    color: #6F675D;
    font-size: 12px;
}
QPushButton#PrimaryAction {
    min-height: 52px;
    min-width: 152px;
    padding: 0 24px;
    border: 2px solid transparent;
    border-radius: 7px;
    background: #8C1D2F;
    color: #FFFFFF;
    font-size: 16px;
    font-weight: 700;
}
QPushButton#PrimaryAction:hover {
    background: #A02439;
}
QPushButton#PrimaryAction:pressed {
    background: #6E1524;
}
QPushButton#PrimaryAction:focus {
    border: 2px solid #1D5FA7;
}
QPushButton#PrimaryAction:disabled {
    background: #E7EBEF;
    color: #7A838D;
}
QPushButton#SecondaryAction {
    min-height: 32px;
    padding: 0 2px;
    border: 1px solid transparent;
    border-bottom: 1px solid #CFC4B3;
    background: transparent;
    color: #6F675D;
    font-size: 12.5px;
}
QPushButton#SecondaryAction:hover {
    color: #23201C;
}
QPushButton#SecondaryAction:focus {
    border: 1px solid #1D5FA7;
    border-bottom: 1px solid #1D5FA7;
}
QPushButton#CancelAction {
    min-height: 44px;
    min-width: 128px;
    padding: 0 22px;
    border: 2px solid #C7A2A9;
    border-radius: 7px;
    background: #FFFCF6;
    color: #8C1D2F;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#CancelAction:hover {
    background: #FDF3F0;
}
QPushButton#CancelAction:focus {
    border: 2px solid #1D5FA7;
}
QPushButton#CancelAction:disabled {
    border-color: #DED5C6;
    color: #7A838D;
    background: #F8F4EC;
}
QToolTip {
    color: #FFFCF6;
    background: #23201C;
    border: none;
    padding: 6px 8px;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 4px 0;
}
QScrollBar::handle:vertical {
    background: #DED5C6;
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
}

} // namespace hustmcl::ui::theme
