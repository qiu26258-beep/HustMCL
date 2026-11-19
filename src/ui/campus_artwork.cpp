#include "campus_artwork.h"

#include "theme.h"

#include <QFontDatabase>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QStringList>
#include <QTransform>
#include <QVector>

namespace hustmcl::ui {

#ifndef HUSTMCL_PUBLIC_RELEASE
namespace {
const QString kCacheKey = QStringLiteral("hustmcl/campus/xiaoke-campus");
}
#else
namespace {

QString mottoFontFamily()
{
    // KaiTi is the preferred school-motto face on Windows; the rest keep the
    // four-character lines legible when a trimmed Windows image lacks it.
    static const QString family = []() {
        const QStringList candidates = {
            QStringLiteral("KaiTi"),
            QStringLiteral("STKaiti"),
            QStringLiteral("FangSong"),
            QStringLiteral("SimSun"),
            QStringLiteral("Microsoft YaHei UI"),
        };
        const QStringList installed = QFontDatabase::families();
        for (const QString& candidate : candidates) {
            if (installed.contains(candidate, Qt::CaseInsensitive)) {
                return candidate;
            }
        }
        return QStringLiteral("Microsoft YaHei UI");
    }();
    return family;
}

QColor token(const char* value)
{
    return QColor(QString::fromLatin1(value));
}

QPainterPath centeredLinePath(const QString& text, const QFont& font)
{
    QPainterPath path;
    path.addText(QPointF(0.0, 0.0), font, text);
    const QRectF bounds = path.boundingRect();
    path.translate(-bounds.center().x(), -bounds.center().y());
    return path;
}

void paintPublicMotto(QPainter& painter, const QRectF& target, CampusArtwork::MottoTone tone)
{
    const bool forest = tone == CampusArtwork::MottoTone::Forest;
    const QColor background = token(forest ? theme::kMottoForest : theme::kMottoPaper);
    const QColor ink = token(forest ? theme::kMottoCream : theme::kMottoInk);
    const QColor hairline = token(forest ? theme::kMottoForestLine : theme::kMottoPaperLine);
    const QColor muted = token(forest ? theme::kMottoForestMuted : theme::kMottoPaperMuted);

    painter.fillRect(target, background);

    const bool compact = target.height() < 90.0;
    QFont font(mottoFontFamily());
    font.setPixelSize(1000);

    QVector<QPainterPath> lines;
    if (compact) {
        lines.append(centeredLinePath(QStringLiteral("明德 厚学 求是 创新"), font));
    } else {
        lines.append(centeredLinePath(QStringLiteral("明德 厚学"), font));
        lines.append(centeredLinePath(QStringLiteral("求是 创新"), font));
    }

    qreal widest = 0.0;
    qreal lineHeight = 0.0;
    for (const QPainterPath& line : lines) {
        const QRectF bounds = line.boundingRect();
        widest = qMax(widest, bounds.width());
        lineHeight = qMax(lineHeight, bounds.height());
    }

    QPainterPath group;
    if (lines.size() == 1) {
        group = lines.first();
    } else {
        const qreal gap = lineHeight * 0.34;
        QPainterPath first = lines.at(0);
        first.translate(0.0, -(lineHeight + gap) / 2.0);
        QPainterPath second = lines.at(1);
        second.translate(0.0, (lineHeight + gap) / 2.0);
        group = first.united(second);
    }

    const QRectF groupBounds = group.boundingRect();
    const qreal availableWidth = target.width() * 0.86;
    const qreal availableHeight = target.height() * (compact ? 0.72 : 0.60);
    const qreal scale = qMin(availableWidth / qMax<qreal>(1.0, widest),
                             availableHeight / qMax<qreal>(1.0, groupBounds.height()));
    const qreal centerY = compact
        ? target.center().y()
        : target.center().y() - target.height() * 0.035;

    QTransform transform;
    transform.translate(target.center().x(), centerY);
    transform.scale(scale, scale);
    transform.translate(-groupBounds.center().x(), -groupBounds.center().y());
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(ink);
    painter.drawPath(transform.map(group));
    painter.restore();

    if (compact) {
        return;
    }

    const qreal ruleY = target.bottom() - target.height() * 0.105;
    const qreal inset = target.width() * 0.17;
    painter.setPen(QPen(hairline, 1.0));
    painter.drawLine(QPointF(target.left() + inset, ruleY),
                     QPointF(target.right() - inset, ruleY));

    QFont attribution(QStringLiteral("Microsoft YaHei UI"));
    attribution.setPixelSize(qBound(9, qRound(target.height() * 0.048), 11));
    painter.setFont(attribution);
    painter.setPen(muted);
    const QRectF attributionRect(target.left() + 2.0, ruleY + 2.0,
                                 target.width() - 4.0, target.bottom() - ruleY - 4.0);
    painter.drawText(attributionRect, Qt::AlignHCenter | Qt::AlignTop,
                     QStringLiteral("华中科技大学校训"));
}
} // namespace
#endif

CampusArtwork::CampusArtwork(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(54);
#ifdef HUSTMCL_PUBLIC_RELEASE
    setAccessibleName(QStringLiteral("华中科技大学校训：明德、厚学、求是、创新"));
    setAccessibleDescription(QStringLiteral("明德 厚学，求是 创新"));
#else
    setAccessibleName(QStringLiteral("华小科校园插画"));
#endif
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

void CampusArtwork::setMottoTone(MottoTone tone)
{
    if (m_mottoTone == tone) {
        return;
    }
    m_mottoTone = tone;
    update();
}

QSize CampusArtwork::sizeHint() const
{
#ifdef HUSTMCL_PUBLIC_RELEASE
    return QSize(226, 208);
#else
    return QSize(226, 280);
#endif
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

    const QRectF target = QRectF(contentsRect()).adjusted(0.5, 0.5, -0.5, -0.5);
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
    paintPublicMotto(painter, target, m_mottoTone);
    painter.setClipping(false);
    painter.setPen(QPen(token(m_mottoTone == MottoTone::Forest
                                  ? theme::kMottoForestLine
                                  : theme::kMottoPaperLine),
                        1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(target, m_cornerRadius, m_cornerRadius);
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

    QLinearGradient fade(0.0, target.top() + target.height() * 0.72, 0.0, target.bottom());
    fade.setColorAt(0.0, QColor(m_overlayColor.red(), m_overlayColor.green(), m_overlayColor.blue(), 0));
    fade.setColorAt(1.0, m_overlayColor);
    painter.fillRect(target, fade);

    painter.setClipping(false);
    painter.setPen(QColor(QStringLiteral("#DED5C6")));
    painter.drawRoundedRect(target, m_cornerRadius, m_cornerRadius);
#endif
}

} // namespace hustmcl::ui
