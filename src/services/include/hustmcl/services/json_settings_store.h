#pragma once
#include <hustmcl/application/launcher_settings.h>

namespace hustmcl::services {

// settings.json with schemaVersion 1; written atomically via QSaveFile.
class JsonSettingsStore final : public application::SettingsStore {
public:
    explicit JsonSettingsStore(QString filePath);
    std::optional<application::LauncherSettings> load() override;
    bool save(const application::LauncherSettings& settings) override;
private:
    QString path_;
};

} // namespace hustmcl::services
