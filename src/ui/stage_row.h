#pragma once

#include <QWidget>

#include <optional>

class QFrame;
class QLabel;
class QProgressBar;

namespace hustmcl::ui {

class StageRow : public QWidget {
public:
    explicit StageRow(QWidget* parent = nullptr);

    void setStage(int number, bool done, bool current, bool last);
    void setDetail(const QString& title, const QString& detail);
    void setProgress(qint64 completedBytes, std::optional<qint64> totalBytes, bool visible);
    void hideProgress();

private:
    void setStageProperty(QLabel* label, const QString& value);
    void repolish(QLabel* label);

    QLabel* m_dot = nullptr;
    QFrame* m_line = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_detail = nullptr;
    QWidget* m_progressBox = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_progressText = nullptr;
    QFrame* m_unknown = nullptr;
    QLabel* m_unknownTitle = nullptr;
    QLabel* m_unknownValue = nullptr;
};

} // namespace hustmcl::ui
