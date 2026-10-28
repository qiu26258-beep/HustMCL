#pragma once
#include <QString>
#include <optional>

namespace hustmcl::application {

inline constexpr int kMinMemoryMb = 1024;
inline constexpr int kMaxMemoryMb = 65536;
inline constexpr int kDefaultMemoryMb = 2048;

// User preferences that survive restarts. Values here are already validated.
struct LauncherSettings {
    QString playerName = QStringLiteral("HustPlayer");
    int memoryMb = kDefaultMemoryMb;
    friend bool operator==(const LauncherSettings&, const LauncherSettings&) = default;
};

[[nodiscard]] inline bool validMemoryMb(int memoryMb) { return memoryMb >= kMinMemoryMb && memoryMb <= kMaxMemoryMb; }

// Persistence port. load() returns nullopt when nothing usable is stored (missing, corrupt,
// or unknown schema); the caller then keeps defaults. save() reports whether data was durably written.
class SettingsStore {
public:
    virtual ~SettingsStore() = default;
    [[nodiscard]] virtual std::optional<LauncherSettings> load() = 0;
    [[nodiscard]] virtual bool save(const LauncherSettings& settings) = 0;
};

} // namespace hustmcl::application
