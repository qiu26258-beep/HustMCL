#include <hustmcl/services/json_settings_store.h>
#include <hustmcl/application/offline_launch.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace hustmcl::services {
using namespace application;
namespace { constexpr int kSchemaVersion = 1; constexpr qint64 kMaxBytes = 64 * 1024; }

JsonSettingsStore::JsonSettingsStore(QString filePath) : path_(std::move(filePath)) {}

std::optional<LauncherSettings> JsonSettingsStore::load()
{
    QFile file(path_);
    if (!file.open(QIODevice::ReadOnly) || file.size() > kMaxBytes) return std::nullopt;
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return std::nullopt;
    const auto object = doc.object();
    if (object.value(QLatin1String("schemaVersion")).toInt(-1) != kSchemaVersion) return std::nullopt;
    LauncherSettings settings;
    // Each field is validated on its own so one bad value does not discard the rest.
    const auto name = object.value(QLatin1String("playerName"));
    if (name.isString() && !std::holds_alternative<OfflineLaunchError>(makeOfflineLaunch({}, name.toString())))
        settings.playerName = name.toString();
    const auto memory = object.value(QLatin1String("memoryMb"));
    if (memory.isDouble() && validMemoryMb(memory.toInt(-1))) settings.memoryMb = memory.toInt();
    const auto offline = object.value(QLatin1String("offlineMode"));
    if (offline.isBool()) settings.offlineMode = offline.toBool();
    return settings;
}

bool JsonSettingsStore::save(const LauncherSettings& settings)
{
    if (!QDir().mkpath(QFileInfo(path_).absolutePath())) return false;
    QSaveFile file(path_);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const QJsonObject object{{QStringLiteral("schemaVersion"), kSchemaVersion},
                             {QStringLiteral("playerName"), settings.playerName},
                             {QStringLiteral("memoryMb"), settings.memoryMb},
                             {QStringLiteral("offlineMode"), settings.offlineMode}};
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return file.commit();
}
} // namespace hustmcl::services
