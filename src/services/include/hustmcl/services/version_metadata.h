#pragma once

#include <hustmcl/core/launch_plan.h>

#include <QByteArray>

namespace hustmcl::services {

using MetadataResult = std::variant<core::VersionMetadata, core::PlanError>;

// Parses caller-supplied bytes. No network or disk access; errors contain schema locations only.
// Scoped to standalone modern release manifests, with strict rule/argument validation.
[[nodiscard]] MetadataResult parseVersionMetadata(const QByteArray& json);

} // namespace hustmcl::services
