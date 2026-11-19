#pragma once

#include "theme.h"

#include <QColor>
#include <QTime>
#include <QWidget>

namespace hustmcl::ui {

// Hardcore pixel/voxel HUST campus diorama renderer:
//   - East 9th Building (东九教学楼四连体建筑群)
//   - HUST South Gate (华科南大门白石朱红牌坊)
//   - Stepped Yujia Mountain (喻家山) & Sycamore Forest (梧桐大道)
//   - Minecraft grass-and-dirt terrain slice with 4 time-of-day sky phases.
// 100% procedural QPainter pixel art cached in QPixmapCache (zero external photos).
class CampusArtwork : public QWidget {
public:
    enum class Style {
        HeroPanorama,
        CompactCrest,
    };

    explicit CampusArtwork(QWidget* parent = nullptr);

    void setDecorationVisible(bool visible);
    bool decorationVisible() const;

    void setDisplayStyle(Style style);
    Style displayStyle() const;

    void setSkyPhase(theme::SkyPhase phase);
    theme::SkyPhase skyPhase() const;
    void syncSkyPhaseToTime(const QTime& localTime = QTime::currentTime());

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_decorationVisible = true;
    Style m_style = Style::HeroPanorama;
    theme::SkyPhase m_skyPhase = theme::resolveSkyPhase();
};

} // namespace hustmcl::ui
