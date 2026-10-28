#include <hustmcl/services/json_settings_store.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
using namespace hustmcl;
using namespace application;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void write(const QString& path, const QByteArray& data) {
    QFile f(path); require(f.open(QIODevice::WriteOnly), "write fixture"); f.write(data);
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir; require(dir.isValid(), "temp dir");
        const auto path = dir.filePath("nested/settings.json");
        services::JsonSettingsStore store(path);
        require(!store.load(), "missing file means no settings");
        require(store.save({"Steve_1", 6144}), "save creates parent directory");
        auto loaded = store.load();
        require(loaded && *loaded == LauncherSettings{"Steve_1", 6144}, "round trip");
        require(!loaded->offlineMode, "old settings default to Microsoft mode");
        require(store.save({"Offline_1", 3072, true}), "save offline preference");
        require(store.load() == LauncherSettings{"Offline_1", 3072, true}, "offline name and mode survive restart");
        write(path, R"({"schemaVersion":1,"playerName":"Old_Name","memoryMb":4096})");
        require(store.load() == LauncherSettings{"Old_Name", 4096, false}, "schema 1 remains backward compatible");
        write(path, R"({"schemaVersion":1,"offlineMode":"true"})");
        require(!store.load()->offlineMode, "non-boolean mode cannot enable offline play");
        write(path, "{not json");
        require(!store.load(), "corrupt file is ignored");
        write(path, R"({"schemaVersion":2,"playerName":"Future","memoryMb":4096})");
        require(!store.load(), "unknown schema is ignored, not guessed");
        write(path, R"({"schemaVersion":1,"playerName":"bad name!","memoryMb":4096})");
        loaded = store.load();
        require(loaded && loaded->playerName == "HustPlayer" && loaded->memoryMb == 4096, "bad name keeps default, good memory kept");
        write(path, R"({"schemaVersion":1,"playerName":"Ok_Name","memoryMb":5})");
        loaded = store.load();
        require(loaded && loaded->playerName == "Ok_Name" && loaded->memoryMb == kDefaultMemoryMb, "bad memory keeps default");
        write(path, R"({"schemaVersion":1,"playerName":7,"memoryMb":"4096"})");
        loaded = store.load();
        require(loaded && *loaded == LauncherSettings{}, "wrong types fall back to defaults");
        std::cout << "json settings store tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
