#include <hustmcl/services/install_plan_input.h>
#include <hustmcl/services/asset_index.h>
#include <hustmcl/services/version_metadata.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace hustmcl::services {
namespace {
std::optional<QByteArray> readLocal(const QString& path, qint64 maximum)
{
    if (!QDir::isAbsolutePath(path) || path.startsWith(QLatin1String("\\\\")) || path.startsWith(QLatin1String("//"))
        || !QFileInfo(path).isFile()) { return {}; }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() < 1 || file.size() > maximum) { return {}; }
    auto bytes = file.read(maximum + 1);
    if (!file.atEnd() || file.error() != QFileDevice::NoError || bytes.size() > maximum) { return {}; }
    return bytes;
}
} // namespace

InstallPlanInputResult loadInstallFilePlan(const QString& versionFile, const QByteArray& versionSha1, const QString& assetIndexFile)
{
    using namespace application;
    const auto versionBytes = readLocal(versionFile, 4 * 1024 * 1024);
    if (!versionBytes) { return PreparationError{PreparationErrorCode::InputUnavailable, {}, {}}; }
    if (QCryptographicHash::hash(*versionBytes, QCryptographicHash::Sha1).toHex() != versionSha1) {
        return PreparationError{PreparationErrorCode::InvalidInput, {}, {}};
    }
    auto parsedVersion = parseVersionMetadata(*versionBytes);
    if (const auto error = std::get_if<core::PlanError>(&parsedVersion)) {
        return PreparationError{PreparationErrorCode::InvalidInput, {}, *error};
    }
    const auto assetBytes = readLocal(assetIndexFile, core::MaximumAssetIndexBytes);
    if (!assetBytes) { return PreparationError{PreparationErrorCode::InputUnavailable, {}, {}}; }
    const auto& version = std::get<core::VersionMetadata>(parsedVersion);
    auto parsedIndex = parseAssetIndex(*assetBytes, version.assetIndex);
    if (const auto error = std::get_if<core::PlanError>(&parsedIndex)) {
        return PreparationError{PreparationErrorCode::InvalidInput, {}, *error};
    }
    auto planned = core::buildInstallFilePlan(version, std::get<core::AssetIndex>(parsedIndex));
    if (const auto error = std::get_if<core::PlanError>(&planned)) {
        return PreparationError{PreparationErrorCode::InvalidPlan, {}, *error};
    }
    return std::get<core::InstallFilePlan>(std::move(planned));
}
} // namespace hustmcl::services
