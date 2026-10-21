#pragma once

#include <QLabel>
#include <QString>

namespace hustmcl::ui {

class ElidedLabel : public QLabel {
public:
    explicit ElidedLabel(QWidget* parent = nullptr);

    void setFullText(const QString& text);
    QString fullText() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void updateElidedText();

    QString m_fullText;
};

} // namespace hustmcl::ui
