#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QString>

#include <array>

namespace hustmcl::ui {

struct InstallStageDefinition {
    QString title;
    QString description;
};

// Single source for the six visible install stages and their phase mapping.
const std::array<InstallStageDefinition, 6>& installStageDefinitions();
int installStageIndex(application::TaskPhase phase);
int installStageNumber(application::TaskPhase phase);
QString installStageFailureText(application::TaskPhase phase);

} // namespace hustmcl::ui
