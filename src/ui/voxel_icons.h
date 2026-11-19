#pragma once

#include <QIcon>
#include <QPixmap>

namespace hustmcl::ui::voxel {

enum class IconKind {
    SouthGateEmblem,
    GrassBlock,
    CraftingTable,
    EnderChest,
    RedstoneGear,
    BookQuill,
    PlayerHead,
    ModpackCrate,
    ThemeAuto,
    ThemeDark,
    ThemeLight,
    WinMinimize,
    WinMaximize,
    WinRestore,
    WinClose,
};

QPixmap renderIcon(IconKind kind, int logicalSize, qreal devicePixelRatio = 1.0);
QIcon icon(IconKind kind, int logicalSize = 18, qreal devicePixelRatio = 1.0);

} // namespace hustmcl::ui::voxel
