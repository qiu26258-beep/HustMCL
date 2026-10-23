#pragma once
#include <hustmcl/core/install_plan.h>
#include <QString>
#include <variant>
namespace hustmcl::services {
enum class LocalGameError { UnsafePath, Metadata, MissingOrCorruptFile };
struct LocalGame {
    core::VersionMetadata version;
    core::InstallFilePlan files;
};
using LocalGameResult = std::variant<LocalGame, LocalGameError>;
// Synchronous, read-only validation for the CLI. Root must be a local absolute directory.
[[nodiscard]] LocalGameResult verifyLocalGame(const QString& root, const QString& versionId);
// Called with the data root lock held; refuses existing reparse-point directories.
[[nodiscard]] bool prepareLocalGameDirectories(const QString& root);
}
