#include <hustmcl/services/local_game.h>
#include <hustmcl/services/install_plan_input.h>
#include <hustmcl/services/version_metadata.h>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace hustmcl::services {
namespace {
bool safePath(QString path) {
    path = QDir::cleanPath(path);
    for (;;) {
        auto native = QDir::toNativeSeparators(path);
        // Published pack roots include stable hashes; library paths may exceed MAX_PATH.
        // Extended local paths retain the same ancestor/reparse-point checks.
        if (!native.startsWith(QLatin1String("\\\\?\\"))) native.prepend(QLatin1String("\\\\?\\"));
        const auto attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()));
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        const auto parent = QFileInfo(path).absolutePath();
        if (parent == path) return true;
        path = parent;
    }
}
}
LocalGameResult verifyLocalGame(const QString& root, const QString& versionId) {
    if (!QRegularExpression("^[A-Za-z]:[/\\\\]").match(root).hasMatch() || !QFileInfo(root).isDir() || !safePath(root)
        || !QRegularExpression("^[A-Za-z0-9_.-]+$").match(versionId).hasMatch() || versionId == "." || versionId == "..")
        return LocalGameError::UnsafePath;
    const QDir directory(root);
    const auto path = directory.filePath("versions/" + versionId + '/' + versionId + ".json");
    QFile metadata(path);
    if (!safePath(path) || !metadata.open(QIODevice::ReadOnly) || metadata.size() > 4 * 1024 * 1024 || metadata.size() < 1)
        return LocalGameError::Metadata;
    const auto bytes = metadata.readAll();
    auto parsed = parseVersionMetadata(bytes);
    if (!std::holds_alternative<core::VersionMetadata>(parsed)) return LocalGameError::Metadata;
    const auto& version = std::get<core::VersionMetadata>(parsed);
    if (version.id != versionId.toStdString()) return LocalGameError::Metadata;
    if (!std::holds_alternative<std::vector<core::PlannedFile>>(core::buildVersionFilePlan(version))) return LocalGameError::Metadata;
    const auto index = directory.filePath("assets/indexes/" + QString::fromStdString(version.assetIndexId) + ".json");
    if (!safePath(index)) return LocalGameError::Metadata;
    auto loaded = loadInstallFilePlan(path, QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex(), index);
    if (!std::holds_alternative<core::InstallFilePlan>(loaded)) return LocalGameError::Metadata;
    const auto& plan = std::get<core::InstallFilePlan>(loaded);
    for (const auto& file : plan.files) {
        const auto local = directory.filePath(QString::fromStdWString(file.relativePath.wstring()));
        QFile content(local);
        if (!safePath(local) || !content.open(QIODevice::ReadOnly) || content.size() != static_cast<qint64>(file.download.size))
            return LocalGameError::MissingOrCorruptFile;
        QCryptographicHash hash(QCryptographicHash::Sha1);
        if (!hash.addData(&content) || hash.result().toHex().toStdString() != file.download.sha1)
            return LocalGameError::MissingOrCorruptFile;
    }
    return LocalGame{std::get<core::VersionMetadata>(std::move(parsed)), std::get<core::InstallFilePlan>(std::move(loaded))};
}
bool prepareLocalGameDirectories(const QString& root) {
    if (!safePath(root) || !QFileInfo(root).isDir()) return false;
    const QDir directory(root);
    for (const auto* relative : {"game", "natives", "launcher-logs"}) {
        const auto path = directory.filePath(QLatin1String(relative));
        if (!QFileInfo::exists(path) && !directory.mkdir(QLatin1String(relative))) return false;
        if (!QFileInfo(path).isDir() || !safePath(path)) return false;
    }
    return true;
}
}
