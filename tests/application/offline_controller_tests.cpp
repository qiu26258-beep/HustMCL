#include <hustmcl/application/offline_controller.h>
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace hustmcl;
using namespace application;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Source : LaunchInputSource {
    std::vector<Completion> calls;
    void load(const QString&, Completion complete) override { calls.push_back(std::move(complete)); }
};
struct Probe : JavaRuntimeProbe {
    std::vector<Completion> calls;
    void probe(const QString&, const std::filesystem::path&, Completion complete) override { calls.push_back(std::move(complete)); }
    void cancel(const QString&) override {}
};
struct Process : GameProcess {
    std::vector<Observer> calls;
    OfflineLaunch identity;
    void start(const QString&, const core::LaunchPlan&, const OfflineLaunch& player, Observer observer) override {
        identity = player; calls.push_back(std::move(observer));
    }
};
LaunchInput input() {
    const core::DownloadDescriptor file{"https://example.org/file", std::string(40, 'a'), 123};
    LaunchInput result;
    auto& v = result.version;
    v.id = "26.2"; v.type = "release"; v.mainClass = "net.minecraft.client.main.Main";
    v.java = {25}; v.client = v.assetIndex = v.loggingFile = file;
    v.assetIndexId = "17"; v.loggingId = "client.xml"; v.loggingArgument = "-Dlog4j.configurationFile=${path}";
    v.jvmArguments = {{{"-cp", "${classpath}"}, {}}};
    v.gameArguments = {{{"--username", "${auth_player_name}"}, {}}};
    result.context.dataRoot = L"D:\\GUI test\\data";
    result.context.gameDirectory = result.context.dataRoot / "game";
    result.context.nativesDirectory = result.context.dataRoot / "natives";
    result.manualJava = L"D:\\runtime\\bin\\java.exe";
    return result;
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        Source source; Probe probe; Process process;
        LaunchPreparation preparation(source, probe);
        {
            OfflineController empty(preparation, process, "D:/empty", "");
            empty.requestPrimaryAction();
            empty.setImportActive(true);
            empty.setImportActive(false);
            empty.requestPrimaryAction();
            require(source.calls.empty() && !empty.homeSnapshot().primaryEnabled, "empty public installation cannot launch after failed or cancelled import");
        }
        OfflineController controller(preparation, process, "D:/GUI test/data", "26.2");
        require(controller.homeSnapshot().offlineMode && !controller.installSnapshot().verified, "initial state must not claim verification");
        require(!controller.setPlayerName("中文") && controller.setPlayerName("Test_Player"), "identity uses application validation");
        controller.setImportActive(true);
        controller.requestPrimaryAction();
        require(source.calls.empty() && !controller.homeSnapshot().primaryEnabled, "import blocks launch in application layer");
        require(!controller.selectInstance({"pack-x", "Pack", "1.21.1", "D:/pack", "1.21.1", "D:/java.exe"}), "import blocks instance switch");
        controller.setImportActive(false);
        controller.requestPrimaryAction(); controller.requestPrimaryAction();
        require(source.calls.size() == 1 && !controller.setPlayerName("Other"), "duplicate launch and active identity edits rejected");
        const auto oldId = controller.installSnapshot().task.taskId;
        controller.requestCancel(oldId);
        require(!controller.homeSnapshot().sessionActive, "cancel loading finishes without process creation");
        controller.requestPrimaryAction();
        source.calls[0](input());
        require(probe.calls.empty(), "cancelled input cannot start Java or game");
        source.calls[1](PreparationError{PreparationErrorCode::InvalidInput, {}, {}});
        require(process.calls.empty() && controller.installSnapshot().task.retryAvailable, "corrupt files stop before process and permit retry");
        controller.requestRetry(oldId);
        require(source.calls.size() == 2, "stale retry ignored");
        controller.requestRetry(controller.installSnapshot().task.taskId);
        source.calls[2](input());
        require(probe.calls.size() == 1 && controller.installSnapshot().verified, "verified input proceeds to actual probe");
        probe.calls[0](core::JavaRuntime{*input().manualJava, 25, core::RuntimeArchitecture::X64});
        require(process.calls.size() == 1 && process.identity.playerName == "Test_Player", "prepared plan resolves current offline identity");
        controller.requestCancel(controller.installSnapshot().task.taskId);
        require(controller.homeSnapshot().sessionActive, "process creation has no destructive cancel");
        process.calls[0]({GameProcessEvent::Kind::Started, "D:/logs/run"});
        require(!controller.homeSnapshot().primaryEnabled && controller.homeSnapshot().logPath == "D:/logs/run", "running state and real log path visible");
        process.calls[0]({GameProcessEvent::Kind::Finished, "D:/logs/run"});
        require(controller.homeSnapshot().primaryEnabled && !controller.homeSnapshot().sessionActive, "normal exit enables next launch");
        controller.requestPrimaryAction();
        process.calls[0]({GameProcessEvent::Kind::Failed, {}, 1, GameProcessError::AbnormalExit});
        require(controller.homeSnapshot().sessionActive && !controller.installSnapshot().task.error, "old process events cannot fail new task");
        source.calls[3](input());
        probe.calls[1](JavaProbeError::FailedToStart);
        require(process.calls.size() == 1 && !controller.homeSnapshot().sessionActive, "bad Java cannot launch game");
        require(controller.selectInstance({"pack-x", "Pack", "1.21.1 Fabric", "D:/pack", "1.21.1", "D:/java.exe"})
            && controller.homeSnapshot().instance.instanceId == "pack-x"
            && controller.homeSnapshot().dataPath == "D:/pack" && controller.installSnapshot().task.taskId.isEmpty(),
            "selecting a published instance resets launch state and does not auto start");
        std::cout << "offline controller scenarios passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
