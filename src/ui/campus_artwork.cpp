#include "campus_artwork.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStyleOption>

namespace hustmcl::ui {

namespace {
const QString kCacheKey = QStringLiteral("hustmcl/campus/xiaoke-campus");
}

CampusArtwork::CampusArtwork(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(120);
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

void CampusArtwork::setCornerRadius(qreal radius)
{
    m_cornerRadius = radius;
    update();
}

void CampusArtwork::setOverlayColor(const QColor& color)
{
    m_overlayColor = color;
    update();
}

void CampusArtwork::setCropFocus(qreal focus)
{
    m_cropFocus = qBound(0.0, focus, 1.0);
    update();
}

QSize CampusArtwork::sizeHint() const
{
    return QSize(226, 280);
}

QPixmap CampusArtwork::sourcePixmap()
{
    QPixmap cached;
    if (QPixmapCache::find(kCacheKey, &cached)) {
        return cached;
    }
    QPixmap loaded(QStringLiteral(":/brand/xiaoke-campus.jpg"));
    if (!loaded.isNull()) {
        QPixmapCache::insert(kCacheKey, loaded);
    }
    return loaded;
}

void CampusArtwork::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF target = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath clip;
    clip.addRoundedRect(target, m_cornerRadius, m_cornerRadius);
    painter.setClipPath(clip);

    if (!m_decorationVisible) {
        painter.fillRect(target, m_overlayColor);
        painter.setClipping(false);
        painter.setPen(QColor(QStringLiteral("#DED5C6")));
        painter.drawRoundedRect(target, m_cornerRadius, m_cornerRadius);
        return;
    }

    const QPixmap source = sourcePixmap();
    if (source.isNull()) {
        painter.fillRect(target, m_overlayColor);
    } else {
        const QSizeF sourceSize = source.size();
        const qreal targetAspect = target.width() / qMax<qreal>(1.0, target.height());
        const qreal sourceAspect = sourceSize.width() / qMax<qreal>(1.0, sourceSize.height());
        QRectF sourceRect(0.0, 0.0, sourceSize.width(), sourceSize.height());
        if (sourceAspect > targetAspect) {
            const qreal croppedWidth = sourceSize.height() * targetAspect;
            const qreal left = (sourceSize.width() - croppedWidth) * m_cropFocus;
            sourceRect = QRectF(left, 0.0, croppedWidth, sourceSize.height());
        } else {
            const qreal croppedHeight = sourceSize.width() / targetAspect;
            const qreal top = (sourceSize.height() - croppedHeight) * 0.5;
            sourceRect = QRectF(0.0, top, sourceSize.width(), croppedHeight);
        }
        painter.drawPixmap(target, source, sourceRect);
    }

    QLinearGradient fade(0.0, target.top() + target.height() * 0.72, 0.0, target.bottom());
    fade.setColorAt(0.0, QColor(m_overlayColor.red(), m_overlayColor.green(), m_overlayColor.blue(), 0));
    fade.setColorAt(1.0, m_overlayColor);
    painter.fillRect(target, fade);

    painter.setClipping(false);
    painter.setPen(QColor(QStringLiteral("#DED5C6")));
    painter.drawRoundedRect(target, m_cornerRadius, m_cornerRadius);
}

} // namespace hustmcl::ui
