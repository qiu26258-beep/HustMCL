#pragma once

#include <QColor>
#include <QPixmap>
#include <QWidget>

namespace hustmcl::ui {

class CampusArtwork : public QWidget {
public:
    explicit CampusArtwork(QWidget* parent = nullptr);

    void setDecorationVisible(bool visible);
    bool decorationVisible() const;

    void setCornerRadius(qreal radius);
    void setOverlayColor(const QColor& color);
    void setCropFocus(qreal focus);

    QSize sizeHint() const override;

private:
    static QPixmap sourcePixmap();
    void paintEvent(QPaintEvent* event) override;

    bool m_decorationVisible = true;
    qreal m_cornerRadius = 10.0;
    QColor m_overlayColor = QColor(QStringLiteral("#FFFCF6"));
    qreal m_cropFocus = 0.82;
};

} // namespace hustmcl::ui
