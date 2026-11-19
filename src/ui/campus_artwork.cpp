#include "campus_artwork.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPixmapCache>

namespace hustmcl::ui {

namespace {

struct SkyColors {
    const char* top;
    const char* mid;
    const char* horizon;
    const char* celestialOuter;
    const char* celestialInner;
    const char* mountainFar;
    const char* forestMid;
    const char* cloudLight;
    const char* cloudShadow;
    bool stars;
    bool litWindows;
};

SkyColors skyColorsFor(theme::SkyPhase phase)
{
    switch (phase) {
    case theme::SkyPhase::Dawn:
        return {
            "#324F73", "#7F9EBA", "#E3C19F",
            "#FFE0A3", "#FFF5D6",
            "#264A45", "#1C3B34",
            "#F5EAE0", "#D9CEC5",
            false, true,
        };
    case theme::SkyPhase::Day:
        return {
            "#1A526B", "#3E918B", "#78BC9A",
            "#FFE37A", "#FFF8CC",
            "#1D4F42", "#163E32",
            "#FFFFFF", "#DCEBE4",
            false, false,
        };
    case theme::SkyPhase::Sunset:
        return {
            "#2B2648", "#97484C", "#E39856",
            "#FFCA6B", "#FFF0B8",
            "#312A3E", "#21252F",
            "#F7C99E", "#C98D76",
            true, true,
        };
    case theme::SkyPhase::Night:
        return {
            "#091221", "#12243B", "#1B3844",
            "#DCE8FF", "#F5FAFF",
            "#0E2422", "#091B18",
            "#2A4554", "#1B303D",
            true, true,
        };
    }
    return skyColorsFor(theme::SkyPhase::Day);
}

void block(QPainter& p, qreal sx, qreal sy, qreal x, qreal y, qreal w, qreal h, const QColor& c)
{
    p.fillRect(QRectF(x * sx, y * sy, w * sx, h * sy), c);
}

void blockHex(QPainter& p, qreal sx, qreal sy, qreal x, qreal y, qreal w, qreal h, const char* hex)
{
    block(p, sx, sy, x, y, w, h, QColor(QString::fromLatin1(hex)));
}

void paintPanoramaScene(QPainter& p, const QSize& pixelSize, theme::SkyPhase phase,
                        theme::ResolvedTheme resolvedTheme)
{
    // Design grid: 210 x 46 voxel units (scales cleanly to wide hero banner)
    const qreal sx = static_cast<qreal>(pixelSize.width()) / 210.0;
    const qreal sy = static_cast<qreal>(pixelSize.height()) / 46.0;
    const SkyColors sky = skyColorsFor(phase);

    // 1. Stepped Minecraft Sky Bands (banded sky feels authentically voxel!)
    QLinearGradient skyGrad(0.0, 0.0, 0.0, pixelSize.height() * 0.85);
    skyGrad.setColorAt(0.0, QColor(QString::fromLatin1(sky.top)));
    skyGrad.setColorAt(0.58, QColor(QString::fromLatin1(sky.mid)));
    skyGrad.setColorAt(1.0, QColor(QString::fromLatin1(sky.horizon)));
    p.fillRect(QRect(QPoint(0, 0), pixelSize), skyGrad);

    // 2. Stars (Sunset / Night)
    if (sky.stars) {
        const int starCoords[][2] = {
            {18, 4}, {36, 9}, {58, 3}, {84, 7}, {108, 4},
            {134, 8}, {158, 3}, {190, 9}, {202, 5}, {148, 12},
        };
        for (const auto& st : starCoords) {
            blockHex(p, sx, sy, st[0], st[1], 1.2, 1.2, "#FFF5C8");
        }
    }

    // 3. Square Minecraft Sun or Moon
    blockHex(p, sx, sy, 174, 4, 9, 9, sky.celestialOuter);
    blockHex(p, sx, sy, 175.5, 5.5, 6, 6, sky.celestialInner);

    // 4. Blocky Clouds
    blockHex(p, sx, sy, 18, 6, 22, 3, sky.cloudShadow);
    blockHex(p, sx, sy, 22, 4, 13, 2, sky.cloudLight);
    blockHex(p, sx, sy, 98, 7, 26, 3, sky.cloudShadow);
    blockHex(p, sx, sy, 104, 5, 14, 2, sky.cloudLight);

    // 5. Layered Stepped Yujia Mountain (喻家山体素层叠)
    blockHex(p, sx, sy, 0, 28, 210, 12, sky.mountainFar);
    blockHex(p, sx, sy, 12, 23, 46, 5, sky.mountainFar);
    blockHex(p, sx, sy, 22, 19, 26, 4, sky.mountainFar);
    blockHex(p, sx, sy, 29, 16, 12, 3, sky.mountainFar);
    blockHex(p, sx, sy, 114, 22, 64, 6, sky.mountainFar);
    blockHex(p, sx, sy, 128, 17, 36, 5, sky.mountainFar);
    blockHex(p, sx, sy, 138, 13, 16, 4, sky.mountainFar);

    // 6. Campus Forest Canopy (森林大学树海)
    blockHex(p, sx, sy, 0, 32, 210, 8, sky.forestMid);
    blockHex(p, sx, sy, 6, 29, 18, 3, sky.forestMid);
    blockHex(p, sx, sy, 102, 29, 22, 3, sky.forestMid);
    blockHex(p, sx, sy, 184, 28, 26, 4, sky.forestMid);

    // 7. LANDMARK A: East 9th Building (东九教学楼四连体建筑群 A/B/C/D 栋)
    // Positioned left-center (x = 46..112), recognizable terraced red-brick & warm-stone academic complex
    const char* wallMain = sky.litWindows ? "#C9BDA9" : "#E5DCCA";
    const char* wallShade = sky.litWindows ? "#9D917F" : "#C5BA9F";
    const char* brickBand = sky.litWindows ? "#8E392C" : "#B34836";
    const char* roofCap = sky.litWindows ? "#586660" : "#7B8C85";
    const char* windowCol = sky.litWindows ? "#F5CA68" : "#2D5B6B";
    const char* windowDim = sky.litWindows ? "#384B52" : "#4A7D8F";

    // Connecting Academic Skybridges (东九连廊)
    blockHex(p, sx, sy, 58, 27, 46, 5, wallShade);
    blockHex(p, sx, sy, 58, 27, 46, 1.2, brickBand);

    // 4 Linked Wings (A, B, C, D 栋错落阶梯屋顶)
    struct Wing { qreal x; qreal topY; qreal w; };
    const Wing wings[] = {
        {46, 22, 14}, // A 栋
        {62, 19, 16}, // B 栋 (主中庭高楼)
        {80, 19, 16}, // C 栋 (主中庭高楼)
        {98, 22, 14}, // D 栋
    };
    for (int i = 0; i < 4; ++i) {
        const Wing& w = wings[i];
        const qreal h = 39.0 - w.topY;
        // Main facade & right shadow edge
        blockHex(p, sx, sy, w.x, w.topY, w.w, h, wallMain);
        blockHex(p, sx, sy, w.x + w.w - 2.2, w.topY, 2.2, h, wallShade);
        // Parapet roof & signature HUST red-brick frieze
        blockHex(p, sx, sy, w.x - 0.8, w.topY - 1.5, w.w + 1.6, 1.5, roofCap);
        blockHex(p, sx, sy, w.x, w.topY + 1.0, w.w, 1.8, brickBand);
        blockHex(p, sx, sy, w.x, 34.5, w.w, 1.4, brickBand);
        // Window grid (3 floors x 3 columns)
        const int cols = (w.w >= 16.0) ? 4 : 3;
        for (int row = 0; row < 3; ++row) {
            const qreal wy = w.topY + 4.2 + row * 3.4;
            if (wy + 2.0 > 35.0) {
                continue;
            }
            for (int col = 0; col < cols; ++col) {
                const qreal wx = w.x + 2.0 + col * 3.0;
                const bool bright = ((i + row + col) % 3 != 0);
                blockHex(p, sx, sy, wx, wy, 1.8, 2.0, bright ? windowCol : windowDim);
            }
        }
    }
    // Central East 9th Plaza Arch Entrance
    blockHex(p, sx, sy, 74, 33, 10, 6, "#2A3A35");
    blockHex(p, sx, sy, 75, 31.5, 8, 1.5, brickBand);

    // 8. LANDMARK B: HUST South Gate (华科南大门白石牌坊, x = 132..176)
    const char* gateWhite = sky.litWindows ? "#E0DACB" : "#F7F2E6";
    const char* gateMid = sky.litWindows ? "#C5BDAB" : "#E8E0CE";
    const char* gateShadow = sky.litWindows ? "#9B9382" : "#C9BEA6";
    // Upper cap towers & main horizontal lintel
    blockHex(p, sx, sy, 138, 13, 6, 2.5, gateWhite);
    blockHex(p, sx, sy, 164, 13, 6, 2.5, gateWhite);
    blockHex(p, sx, sy, 134, 15.5, 40, 3.0, gateWhite);
    blockHex(p, sx, sy, 132, 18.5, 44, 4.0, gateMid);
    blockHex(p, sx, sy, 132, 22.0, 44, 1.2, gateShadow);
    // Iconic HUST South Gate Crimson Name Beam (朱红校名匾额)
    blockHex(p, sx, sy, 141, 19.3, 26, 2.2, "#C9422D");
    blockHex(p, sx, sy, 143, 19.8, 22, 0.9, "#EA6E59");
    // Left & Right White Pillars with plinths
    blockHex(p, sx, sy, 138, 23.2, 6.5, 16.0, gateWhite);
    blockHex(p, sx, sy, 143, 23.2, 1.5, 16.0, gateShadow);
    blockHex(p, sx, sy, 136.5, 36.8, 9.5, 2.5, gateMid);

    blockHex(p, sx, sy, 163.5, 23.2, 6.5, 16.0, gateWhite);
    blockHex(p, sx, sy, 168.5, 23.2, 1.5, 16.0, gateShadow);
    blockHex(p, sx, sy, 162.0, 36.8, 9.5, 2.5, gateMid);
    // Warm glowstone gate lantern at Sunset/Night
    if (sky.litWindows) {
        blockHex(p, sx, sy, 151.5, 24.5, 5.0, 1.6, "#F5CA68");
    }

    // 9. Voxel Sycamore Trees (左右梧桐树冠 + 金绿像素叶)
    // Left Sycamore
    blockHex(p, sx, sy, 14, 29, 3.5, 10.5, "#5A3B24");
    blockHex(p, sx, sy, 7, 21, 17, 8.5, "#267341");
    blockHex(p, sx, sy, 10, 17, 11, 4.5, "#379654");
    blockHex(p, sx, sy, 9, 22, 4, 3, "#5EC469");
    blockHex(p, sx, sy, 18, 19, 3.5, 3, "#E0B64A");
    // Mid-Right Sycamore between East 9th and South Gate
    blockHex(p, sx, sy, 120, 31, 3.0, 8.5, "#5A3B24");
    blockHex(p, sx, sy, 115, 24, 13, 7.0, "#23693C");
    blockHex(p, sx, sy, 117, 21, 9, 3.5, "#379654");
    blockHex(p, sx, sy, 123, 25, 3, 2.5, "#E0B64A");
    // Far Right Sycamore
    blockHex(p, sx, sy, 190, 28, 3.8, 11.5, "#5A3B24");
    blockHex(p, sx, sy, 182, 20, 19, 8.5, "#267341");
    blockHex(p, sx, sy, 185, 16, 13, 4.5, "#379654");
    blockHex(p, sx, sy, 185, 21, 4, 3, "#E5BC52");
    blockHex(p, sx, sy, 195, 22, 3.5, 3, "#5EC469");

    // Floating Pixel Sycamore Leaves
    blockHex(p, sx, sy, 34, 15, 1.8, 1.2, "#7AE688");
    blockHex(p, sx, sy, 127, 15, 1.6, 1.2, "#E8B949");
    blockHex(p, sx, sy, 178, 17, 1.5, 1.2, "#7AE688");

    // 10. Foreground Minecraft Grass & Dirt Cross-Section + Stone Brick Avenue
    blockHex(p, sx, sy, 0, 39.2, 210, 2.4, "#42BA55");
    blockHex(p, sx, sy, 0, 39.2, 210, 0.9, "#6FE680");
    blockHex(p, sx, sy, 0, 41.6, 210, 4.5, "#61412A");
    // Dirt dark pebbles
    const int pebbles[][2] = {{10, 43}, {32, 44}, {66, 43}, {98, 44}, {122, 43}, {182, 44}, {198, 43}};
    for (const auto& pb : pebbles) {
        blockHex(p, sx, sy, pb[0], pb[1], 4.5, 1.4, "#472E1C");
    }
    // Grass pixel overhangs
    const int hangs[][2] = {{6, 4}, {26, 6}, {54, 5}, {88, 6}, {114, 5}, {180, 7}, {196, 5}};
    for (const auto& hg : hangs) {
        blockHex(p, sx, sy, hg[0], 41.6, hg[1], 1.4, "#319641");
    }
    // Stone Brick Walkway under South Gate & East 9th Plaza
    blockHex(p, sx, sy, 134, 39.2, 40, 2.4, "#9BA6A0");
    blockHex(p, sx, sy, 136, 39.6, 8, 1.4, "#BCC7C1");
    blockHex(p, sx, sy, 146, 39.6, 9, 1.4, "#BCC7C1");
    blockHex(p, sx, sy, 157, 39.6, 8, 1.4, "#BCC7C1");
    blockHex(p, sx, sy, 167, 39.6, 5, 1.4, "#BCC7C1");

    // 11. Subtle left-side readability vignette so overlaid hero title stays ultra-crisp
    QLinearGradient shade(0.0, 0.0, pixelSize.width(), 0.0);
    if (resolvedTheme == theme::ResolvedTheme::Dark) {
        shade.setColorAt(0.0, QColor(8, 16, 13, 195));
        shade.setColorAt(0.45, QColor(8, 16, 13, 95));
        shade.setColorAt(1.0, QColor(8, 16, 13, 60));
    } else {
        shade.setColorAt(0.0, QColor(12, 28, 22, 175));
        shade.setColorAt(0.45, QColor(12, 28, 22, 65));
        shade.setColorAt(1.0, QColor(12, 28, 22, 35));
    }
    p.fillRect(QRect(QPoint(0, 0), pixelSize), shade);
}

void paintCompactCrest(QPainter& p, const QSize& pixelSize, theme::SkyPhase phase)
{
    paintPanoramaScene(p, pixelSize, phase, theme::ResolvedTheme::Dark);
}

} // namespace

CampusArtwork::CampusArtwork(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(88);
    setAccessibleName(QStringLiteral("华中科技大学东九教学楼与南大门体素全景"));
    setAccessibleDescription(QStringLiteral("融合华中科技大学南大门、东九教学楼、喻家山与我的世界方块景观"));
}

void CampusArtwork::setDecorationVisible(bool visible)
{
    if (m_decorationVisible == visible) {
        return;
    }
    m_decorationVisible = visible;
    update();
}

bool CampusArtwork::decorationVisible() const
{
    return m_decorationVisible;
}

void CampusArtwork::setDisplayStyle(Style style)
{
    if (m_style == style) {
        return;
    }
    m_style = style;
    update();
}

CampusArtwork::Style CampusArtwork::displayStyle() const
{
    return m_style;
}

void CampusArtwork::setSkyPhase(theme::SkyPhase phase)
{
    if (m_skyPhase == phase) {
        return;
    }
    m_skyPhase = phase;
    update();
}

theme::SkyPhase CampusArtwork::skyPhase() const
{
    return m_skyPhase;
}

void CampusArtwork::syncSkyPhaseToTime(const QTime& localTime)
{
    setSkyPhase(theme::resolveSkyPhase(localTime));
}

QSize CampusArtwork::sizeHint() const
{
    return m_style == Style::HeroPanorama ? QSize(760, 152) : QSize(184, 96);
}

void CampusArtwork::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QRect target = rect();
    if (target.width() <= 4 || target.height() <= 4) {
        return;
    }

    const auto& pal = theme::currentPalette();
    if (!m_decorationVisible) {
        painter.fillRect(target, theme::color(pal.surfaceAlt));
        painter.setPen(QPen(theme::color(pal.border), 2));
        painter.drawRect(target.adjusted(1, 1, -1, -1));
        return;
    }

    const qreal dpr = qMax<qreal>(1.0, devicePixelRatioF());
    const QSize pixelSize(qMax(16, qRound(target.width() * dpr)),
                          qMax(16, qRound(target.height() * dpr)));
    const QString cacheKey = QStringLiteral("hustmcl/voxel_campus/%1/%2/%3/%4x%5")
                                 .arg(static_cast<int>(m_style))
                                 .arg(static_cast<int>(m_skyPhase))
                                 .arg(static_cast<int>(theme::activeTheme()))
                                 .arg(pixelSize.width())
                                 .arg(pixelSize.height());
    QPixmap rendered;
    if (!QPixmapCache::find(cacheKey, &rendered)) {
        rendered = QPixmap(pixelSize);
        rendered.fill(Qt::transparent);
        QPainter offscreen(&rendered);
        offscreen.setRenderHint(QPainter::Antialiasing, false);
        if (m_style == Style::HeroPanorama) {
            paintPanoramaScene(offscreen, pixelSize, m_skyPhase, theme::activeTheme());
        } else {
            paintCompactCrest(offscreen, pixelSize, m_skyPhase);
        }
        rendered.setDevicePixelRatio(dpr);
        QPixmapCache::insert(cacheKey, rendered);
    }

    painter.drawPixmap(target, rendered);

    // Crisp 2px pixel bevel frame around the diorama
    painter.fillRect(QRect(target.left(), target.top(), target.width(), 2), theme::color(pal.bevelHi));
    painter.fillRect(QRect(target.left(), target.top(), 2, target.height()), theme::color(pal.bevelHi));
    painter.fillRect(QRect(target.left(), target.bottom() - 1, target.width(), 2), theme::color(pal.bevelLo));
    painter.fillRect(QRect(target.right() - 1, target.top(), 2, target.height()), theme::color(pal.bevelLo));
}

} // namespace hustmcl::ui
