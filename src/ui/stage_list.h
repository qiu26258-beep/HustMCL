#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QWidget>

#include <vector>

namespace hustmcl::ui {

class StageRow;

class StageList : public QWidget {
public:
    explicit StageList(QWidget* parent = nullptr);

    void setTask(const application::TaskSnapshot& task);

private:
    static int phaseIndex(application::TaskPhase phase);
    std::vector<StageRow*> m_rows;
};

} // namespace hustmcl::ui
