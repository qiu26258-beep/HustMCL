#include "voxel_icons.h"

#include "theme.h"

#include <QColor>
#include <QPainter>
#include <QPixmapCache>

namespace hustmcl::ui::voxel {

namespace {

void px(QPainter& p, qreal unit, int x, int y, int w, int h, const char* hex)
{
    p.fillRect(QRectF(x * unit, y * unit, w * unit, h * unit), QColor(QString::fromLatin1(hex)));
}

void drawSouthGateEmblem(QPainter& p, qreal u)
{
    // 16x16 pixel emblem: HUST South Gate with crimson beam on Minecraft grass block
    px(p, u, 0, 0, 16, 16, "#103D2F");
    // Pixel border
    px(p, u, 0, 0, 16, 1, "#2A6B53");
    px(p, u, 0, 0, 1, 16, "#2A6B53");
    px(p, u, 0, 15, 16, 1, "#072018");
    px(p, u, 15, 0, 1, 16, "#072018");
    // Dirt & grass base
    px(p, u, 2, 12, 12, 3, "#7B5032");
    px(p, u, 4, 13, 2, 1, "#5C3921");
    px(p, u, 10, 14, 3, 1, "#5C3921");
    px(p, u, 2, 10, 12, 2, "#42BE57");
    px(p, u, 2, 10, 12, 1, "#6BE37C");
    px(p, u, 3, 12, 2, 1, "#309943");
    px(p, u, 8, 12, 3, 1, "#309943");
    px(p, u, 12, 12, 1, 1, "#309943");
    // South Gate top cap & pillars
    px(p, u, 3, 2, 2, 1, "#F6F1E5");
    px(p, u, 11, 2, 2, 1, "#F6F1E5");
    px(p, u, 2, 3, 12, 2, "#F6F1E5");
    px(p, u, 3, 5, 10, 1, "#D4CBB6");
    // Crimson HUST beam
    px(p, u, 4, 4, 8, 1, "#D14632");
    // Pillars
    px(p, u, 4, 6, 2, 4, "#F2ECE0");
    px(p, u, 5, 6, 1, 4, "#D4CBB6");
    px(p, u, 10, 6, 2, 4, "#F2ECE0");
    px(p, u, 11, 6, 1, 4, "#D4CBB6");
    // Pixel sycamore leaf inside gate
    px(p, u, 7, 7, 2, 2, "#7AE688");
    px(p, u, 8, 6, 1, 1, "#B2F7B6");
}

void drawGrassBlock(QPainter& p, qreal u)
{
    px(p, u, 1, 1, 14, 14, "#0B1410");
    px(p, u, 2, 6, 12, 8, "#855736");
    px(p, u, 4, 9, 2, 2, "#633E24");
    px(p, u, 10, 11, 2, 2, "#633E24");
    px(p, u, 7, 12, 2, 1, "#9E6A44");
    px(p, u, 2, 2, 12, 4, "#44C458");
    px(p, u, 2, 2, 12, 1, "#7CF08A");
    px(p, u, 3, 6, 2, 2, "#329943");
    px(p, u, 7, 6, 3, 1, "#329943");
    px(p, u, 11, 6, 2, 2, "#329943");
}

void drawCraftingTable(QPainter& p, qreal u)
{
    px(p, u, 1, 1, 14, 14, "#2B190C");
    px(p, u, 2, 2, 12, 12, "#9F6B40");
    px(p, u, 2, 2, 12, 4, "#CC9762");
    px(p, u, 5, 2, 1, 4, "#6B4223");
    px(p, u, 10, 2, 1, 4, "#6B4223");
    px(p, u, 2, 6, 12, 1, "#57341B");
    px(p, u, 4, 8, 2, 5, "#57341B");
    px(p, u, 9, 8, 3, 3, "#B8C7CF");
    px(p, u, 10, 11, 1, 2, "#57341B");
}

void drawEnderChest(QPainter& p, qreal u)
{
    px(p, u, 1, 2, 14, 12, "#0B2421");
    px(p, u, 2, 3, 12, 10, "#1D5E55");
    px(p, u, 2, 3, 12, 3, "#2A8074");
    px(p, u, 2, 6, 12, 1, "#0E332E");
    px(p, u, 7, 5, 2, 3, "#F0C75E");
    px(p, u, 4, 8, 2, 2, "#52E3B1");
    px(p, u, 10, 9, 2, 2, "#52E3B1");
}

void drawRedstoneGear(QPainter& p, qreal u)
{
    px(p, u, 6, 1, 4, 2, "#D14A34");
    px(p, u, 6, 13, 4, 2, "#D14A34");
    px(p, u, 1, 6, 2, 4, "#D14A34");
    px(p, u, 13, 6, 2, 4, "#D14A34");
    px(p, u, 3, 3, 2, 2, "#B53824");
    px(p, u, 11, 3, 2, 2, "#B53824");
    px(p, u, 3, 11, 2, 2, "#B53824");
    px(p, u, 11, 11, 2, 2, "#B53824");
    px(p, u, 4, 4, 8, 8, "#E3624A");
    px(p, u, 6, 6, 4, 4, "#101916");
    px(p, u, 7, 7, 2, 2, "#FFAB99");
}

void drawBookQuill(QPainter& p, qreal u)
{
    px(p, u, 2, 1, 10, 14, "#3A2012");
    px(p, u, 3, 2, 8, 12, "#8E5230");
    px(p, u, 4, 3, 6, 10, "#B87248");
    px(p, u, 5, 4, 4, 8, "#F3E9D4");
    px(p, u, 6, 6, 2, 1, "#5C4738");
    px(p, u, 6, 8, 2, 1, "#5C4738");
    // Golden quill
    px(p, u, 12, 2, 2, 4, "#E8B949");
    px(p, u, 11, 5, 2, 2, "#F6F1E5");
    px(p, u, 10, 7, 1, 1, "#C94530");
}

void drawPlayerHead(QPainter& p, qreal u)
{
    // 16x16 pixel crafter face (2px per face pixel)
    px(p, u, 0, 0, 16, 16, "#E2B791");
    // Hair
    px(p, u, 0, 0, 16, 4, "#2A1F1A");
    px(p, u, 0, 4, 2, 2, "#2A1F1A");
    px(p, u, 14, 4, 2, 2, "#2A1F1A");
    // Eyes (HUST forest emerald iris)
    px(p, u, 2, 8, 2, 2, "#FFFFFF");
    px(p, u, 4, 8, 2, 2, "#247A4B");
    px(p, u, 10, 8, 2, 2, "#247A4B");
    px(p, u, 12, 8, 2, 2, "#FFFFFF");
    // Nose & smile
    px(p, u, 6, 10, 4, 2, "#C4916B");
    px(p, u, 6, 12, 4, 2, "#875338");
    px(p, u, 4, 12, 2, 1, "#A87454");
    px(p, u, 10, 12, 2, 1, "#A87454");
}

void drawModpackCrate(QPainter& p, qreal u)
{
    px(p, u, 1, 2, 14, 12, "#382212");
    px(p, u, 2, 3, 12, 10, "#B37A44");
    px(p, u, 2, 3, 12, 3, "#D69C62");
    px(p, u, 6, 3, 4, 10, "#E8B949");
    px(p, u, 7, 6, 2, 3, "#7A4C22");
    px(p, u, 2, 6, 4, 1, "#7A4C22");
    px(p, u, 10, 6, 4, 1, "#7A4C22");
}

void drawThemeAuto(QPainter& p, qreal u)
{
    // Half sun (left) + half moon (right) pixel badge
    px(p, u, 3, 4, 5, 8, "#F2C85B");
    px(p, u, 4, 3, 4, 10, "#FFE28A");
    px(p, u, 1, 7, 2, 2, "#F2C85B");
    px(p, u, 8, 3, 4, 10, "#5B9CE8");
    px(p, u, 10, 5, 3, 6, "#14263E");
    px(p, u, 12, 3, 2, 2, "#DCEEFF");
}

void drawThemeDark(QPainter& p, qreal u)
{
    // Pixel crescent moon & star
    px(p, u, 4, 2, 6, 12, "#E8F0FF");
    px(p, u, 3, 4, 8, 8, "#E8F0FF");
    px(p, u, 7, 4, 5, 7, "#0B1411");
    px(p, u, 12, 3, 2, 2, "#F2C85B");
    px(p, u, 11, 10, 2, 2, "#7AE688");
}

void drawThemeLight(QPainter& p, qreal u)
{
    // Pixel Minecraft square sun
    px(p, u, 4, 4, 8, 8, "#F2B830");
    px(p, u, 5, 5, 6, 6, "#FFE680");
    px(p, u, 7, 1, 2, 2, "#F2B830");
    px(p, u, 7, 13, 2, 2, "#F2B830");
    px(p, u, 1, 7, 2, 2, "#F2B830");
    px(p, u, 13, 7, 2, 2, "#F2B830");
}

void drawWindowGlyph(QPainter& p, qreal u, IconKind kind)
{
    const char* ink = theme::currentPalette().titleText;
    switch (kind) {
    case IconKind::WinMinimize:
        px(p, u, 3, 8, 10, 2, ink);
        break;
    case IconKind::WinMaximize:
        px(p, u, 3, 3, 10, 2, ink);
        px(p, u, 3, 11, 10, 2, ink);
        px(p, u, 3, 3, 2, 10, ink);
        px(p, u, 11, 3, 2, 10, ink);
        break;
    case IconKind::WinRestore:
        px(p, u, 5, 2, 8, 2, ink);
        px(p, u, 11, 2, 2, 8, ink);
        px(p, u, 3, 5, 8, 2, ink);
        px(p, u, 3, 11, 8, 2, ink);
        px(p, u, 3, 5, 2, 8, ink);
        px(p, u, 9, 5, 2, 8, ink);
        break;
    case IconKind::WinClose:
        for (int i = 0; i < 8; ++i) {
            px(p, u, 3 + i, 3 + i, 2, 2, ink);
            px(p, u, 11 - i, 3 + i, 2, 2, ink);
        }
        break;
    default:
        break;
    }
}

} // namespace

QPixmap renderIcon(IconKind kind, int logicalSize, qreal devicePixelRatio)
{
    const int size = qMax(8, logicalSize);
    const qreal dpr = qMax<qreal>(1.0, devicePixelRatio);
    const QString key = QStringLiteral("hustmcl/voxel_icon/%1/%2/%3/%4")
                            .arg(static_cast<int>(kind))
                            .arg(size)
                            .arg(qRound(dpr * 100.0))
                            .arg(static_cast<int>(theme::activeTheme()));
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }

    const int pixelDim = qMax(16, qRound(size * dpr));
    QPixmap pixmap(pixelDim, pixelDim);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const qreal unit = static_cast<qreal>(pixelDim) / 16.0;

    switch (kind) {
    case IconKind::SouthGateEmblem:
        drawSouthGateEmblem(painter, unit);
        break;
    case IconKind::GrassBlock:
        drawGrassBlock(painter, unit);
        break;
    case IconKind::CraftingTable:
        drawCraftingTable(painter, unit);
        break;
    case IconKind::EnderChest:
        drawEnderChest(painter, unit);
        break;
    case IconKind::RedstoneGear:
        drawRedstoneGear(painter, unit);
        break;
    case IconKind::BookQuill:
        drawBookQuill(painter, unit);
        break;
    case IconKind::PlayerHead:
        drawPlayerHead(painter, unit);
        break;
    case IconKind::ModpackCrate:
        drawModpackCrate(painter, unit);
        break;
    case IconKind::ThemeAuto:
        drawThemeAuto(painter, unit);
        break;
    case IconKind::ThemeDark:
        drawThemeDark(painter, unit);
        break;
    case IconKind::ThemeLight:
        drawThemeLight(painter, unit);
        break;
    case IconKind::WinMinimize:
    case IconKind::WinMaximize:
    case IconKind::WinRestore:
    case IconKind::WinClose:
        drawWindowGlyph(painter, unit, kind);
        break;
    }

    pixmap.setDevicePixelRatio(dpr);
    QPixmapCache::insert(key, pixmap);
    return pixmap;
}

QIcon icon(IconKind kind, int logicalSize, qreal devicePixelRatio)
{
    return QIcon(renderIcon(kind, logicalSize, devicePixelRatio));
}

} // namespace hustmcl::ui::voxel
