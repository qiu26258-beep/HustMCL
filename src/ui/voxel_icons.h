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

// Renders a custom 3D Minecraft-block pixel wordmark for "HustMCL"
// ("Hust" in HUST sandstone ivory, "MCL" in Minecraft grass emerald)
// with crisp obsidian outline and 2-voxel drop extrusion.
QPixmap renderWordmark(int pixelScale = 3, qreal devicePixelRatio = 1.0);

} // namespace hustmcl::ui::voxel
