#include "campus_artwork.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStyleOption>

namespace hustmcl::ui {

#ifndef HUSTMCL_PUBLIC_RELEASE
namespace {
const QString kCacheKey = QStringLiteral("hustmcl/campus/xiaoke-campus");
}
#else
namespace {

void paintPublicCampus(QPainter& painter, const QRectF& target)
{
    const qreal width = target.width();
    const qreal height = target.height();
    const auto x = [&](qreal value) { return target.left() + width * value; };
    const auto y = [&](qreal value) { return target.top() + height * value; };

    QLinearGradient sky(target.topLeft(), target.bottomLeft());
    sky.setColorAt(0.0, QColor(QStringLiteral("#F8F1E6")));
    sky.setColorAt(0.62, QColor(QStringLiteral("#EAF0E4")));
    sky.setColorAt(1.0, QColor(QStringLiteral("#DDE7D8")));
    painter.fillRect(target, sky);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#E8DAC5")));
    painter.drawRoundedRect(QRectF(x(0.43), y(0.30), width * 0.48, height * 0.38), 3.0, 3.0);
    painter.setBrush(QColor(QStringLiteral("#BDA88F")));
    painter.drawRect(QRectF(x(0.40), y(0.27), width * 0.54, height * 0.045));

    painter.setBrush(QColor(QStringLiteral("#FBF5EA")));
    painter.setPen(QPen(QColor(QStringLiteral("#D5C4AD")), qMax(1.0, width * 0.004)));
    const qreal windowWidth = width * 0.10;
    const qreal windowHeight = height * 0.065;
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 3; ++column) {
            painter.drawRoundedRect(
                QRectF(x(0.49) + column * width * 0.14, y(0.36) + row * height * 0.12,
                       windowWidth, windowHeight),
                1.5, 1.5);
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#8C1D2F")));
    const QRectF gate(x(0.16), y(0.45), width * 0.28, height * 0.42);
    painter.drawEllipse(QRectF(gate.left(), gate.top(), gate.width(), gate.width()));
    painter.drawRect(QRectF(gate.left(), gate.top() + gate.width() * 0.5,
                            gate.width(), gate.height() - gate.width() * 0.5));

    const QRectF opening(x(0.20), y(0.52), width * 0.20, height * 0.35);
    painter.setBrush(QColor(QStringLiteral("#F7EFDF")));
    painter.drawEllipse(QRectF(opening.left(), opening.top(), opening.width(), opening.width()));
    painter.drawRect(QRectF(opening.left(), opening.top() + opening.width() * 0.5,
                            opening.width(), opening.height() - opening.width() * 0.5));

    painter.setBrush(QColor(QStringLiteral("#B8957E")));
    painter.drawRect(QRectF(x(0.14), y(0.86), width * 0.32, height * 0.025));
    painter.setBrush(QColor(QStringLiteral("#D9CBB8")));
    painter.drawRoundedRect(QRectF(x(0.10), y(0.89), width * 0.40, height * 0.025), 2.0, 2.0);
    painter.setBrush(QColor(QStringLiteral("#CDBCA6")));
    painter.drawRoundedRect(QRectF(x(0.06), y(0.92), width * 0.48, height * 0.025), 2.0, 2.0);

    painter.setPen(QPen(QColor(QStringLiteral("#6B5542")), qMax(2.0, width * 0.03),
                        Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(x(0.66), y(0.88)), QPointF(x(0.66), y(0.35)));
    painter.drawLine(QPointF(x(0.66), y(0.52)), QPointF(x(0.57), y(0.42)));
    painter.drawLine(QPointF(x(0.66), y(0.46)), QPointF(x(0.76), y(0.37)));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(91, 122, 88, 220));
    painter.drawEllipse(QPointF(x(0.34), y(0.20)), width * 0.22, height * 0.13);
    painter.drawEllipse(QPointF(x(0.57), y(0.15)), width * 0.19, height * 0.12);
    painter.drawEllipse(QPointF(x(0.79), y(0.20)), width * 0.18, height * 0.11);
    painter.setBrush(QColor(127, 155, 120, 190));
    painter.drawEllipse(QPointF(x(0.25), y(0.13)), width * 0.12, height * 0.08);
    painter.drawEllipse(QPointF(x(0.67), y(0.11)), width * 0.10, height * 0.07);
    painter.drawEllipse(QPointF(x(0.90), y(0.13)), width * 0.08, height * 0.06);

    painter.setBrush(QColor(169, 191, 160, 210));
    painter.save();
    painter.translate(x(0.12), y(0.33));
    painter.rotate(-28.0);
    painter.drawEllipse(QRectF(-width * 0.055, -height * 0.018,
                               width * 0.11, height * 0.036));
    painter.restore();
    painter.save();
    painter.translate(x(0.87), y(0.35));
    painter.rotate(24.0);
    painter.drawEllipse(QRectF(-width * 0.05, -height * 0.016,
                               width * 0.10, height * 0.032));
    painter.restore();

    painter.setPen(QPen(QColor(QStringLiteral("#D2C2AC")), qMax(1.0, width * 0.006)));
    painter.drawLine(QPointF(target.left(), y(0.88)), QPointF(target.right(), y(0.88)));
}
}
#endif

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

#ifndef HUSTMCL_PUBLIC_RELEASE
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
#endif

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

#ifdef HUSTMCL_PUBLIC_RELEASE
    paintPublicCampus(painter, target);
#else
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
#endif

    QLinearGradient fade(0.0, target.top() + target.height() * 0.72, 0.0, target.bottom());
    fade.setColorAt(0.0, QColor(m_overlayColor.red(), m_overlayColor.green(), m_overlayColor.blue(), 0));
    fade.setColorAt(1.0, m_overlayColor);
    painter.fillRect(target, fade);

    painter.setClipping(false);
    painter.setPen(QColor(QStringLiteral("#DED5C6")));
    painter.drawRoundedRect(target, m_cornerRadius, m_cornerRadius);
}

} // namespace hustmcl::ui
