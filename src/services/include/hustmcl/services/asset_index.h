#pragma once

#include <hustmcl/core/install_plan.h>

#include <QByteArray>

namespace hustmcl::services {
using AssetIndexResult = std::variant<core::AssetIndex, core::PlanError>;

// Caller supplies trusted version reference; raw size and SHA-1 are checked before JSON parsing.
// Modern content-addressed assets only. No I/O, credentials, legacy asset copying or extraction.
[[nodiscard]] AssetIndexResult parseAssetIndex(const QByteArray& json, const core::DownloadDescriptor& expected);
} // namespace hustmcl::services
