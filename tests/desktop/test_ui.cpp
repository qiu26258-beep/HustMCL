#include "demo_controller.h"
#include "main_window.h"

#include "checks_page.h"
#include "content_market_page.h"
#include "copy.h"
#include "home_page.h"
#include "install_page.h"
#include "install_stages.h"
#include "logs_page.h"
#include "hustmcl/application/content_market.h"
#include "hustmcl/application/launch_preparation.h"
#include "hustmcl/application/modpack_import.h"
#include "hustmcl/application/offline_controller.h"
#include "primary_action_bar.h"
#include "settings_page.h"
#include "sidebar.h"
#include "theme.h"

#include <QApplication>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmapCache>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalSpy>
#include <QStyle>
#include <QStyleFactory>
#include <QTest>
#include <QTimer>
#include <QUrl>

#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace application = hustmcl::application;
namespace core = hustmcl::core;

namespace {

QString joinedLabelText(QWidget& widget)
{
    QStringList texts;
    for (QLabel* label : widget.findChildren<QLabel*>()) {
        texts.append(label->text());
    }
    return texts.join(QLatin1Char('\n'));
}

QString visibleLabelText(QWidget& widget)
{
    QStringList texts;
    for (QLabel* label : widget.findChildren<QLabel*>()) {
        if (label->isVisible()) {
            texts.append(label->text());
        }
    }
    return texts.join(QLatin1Char('\n'));
}

QProgressBar* visibleProgressBar(QWidget& widget)
{
    for (QProgressBar* bar : widget.findChildren<QProgressBar*>()) {
        if (bar->isVisible()) {
            return bar;
        }
    }
    return nullptr;
}

application::InstallSnapshot downloadingSnapshot(qint64 completedBytes,
                                                 std::optional<qint64> totalBytes)
{
    application::InstallSnapshot snapshot;
    snapshot.instanceDisplayName = QStringLiteral("校园生存");
    snapshot.versionLabel = QStringLiteral("1.21.4");
    snapshot.task.taskId = QStringLiteral("task-progress");
    snapshot.task.phase = application::TaskPhase::Downloading;
    snapshot.task.state = application::TaskState::Running;
    snapshot.task.completedBytes = completedBytes;
    snapshot.task.totalBytes = totalBytes;
    snapshot.task.cancelAvailable = true;
    snapshot.task.phaseDetail = QStringLiteral("正在下载游戏资源");
    return snapshot;
}

QLabel* stageSub(hustmcl::ui::InstallPage& page, int index)
{
    auto* row = page.findChild<QWidget*>(QStringLiteral("StageRow%1").arg(index));
    return row ? row->findChild<QLabel*>(QStringLiteral("StageSub")) : nullptr;
}

QPushButton* buttonWithText(QWidget& widget, const QString& text)
{
    for (QPushButton* button : widget.findChildren<QPushButton*>()) {
        if (button->text() == text) {
            return button;
        }
    }
    return nullptr;
}

QPushButton* modpackPickButton(QWidget& widget)
{
    for (QPushButton* button : widget.findChildren<QPushButton*>()) {
        if (button->accessibleName() == QStringLiteral("选择整合包文件")) {
            return button;
        }
    }
    return nullptr;
}

void sendUrlDrop(hustmcl::ui::HomePage& page, const QList<QUrl>& urls)
{
    page.handleDroppedUrls(urls);
}

void sendNativeUrlDrop(hustmcl::ui::HomePage& page, const QList<QUrl>& urls)
{
    QMimeData mime;
    mime.setUrls(urls);
    const QPoint position(8, 8);
    QDragEnterEvent enter(position, Qt::CopyAction, &mime,
                          Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&page, &enter);
    QDragMoveEvent move(position, Qt::CopyAction, &mime,
                        Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&page, &move);
    QDropEvent drop(QPointF(position), Qt::CopyAction, &mime,
                    Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&page, &drop);
}

// Deterministic test ports for the real application controller. No test uses
// a production service here; the OfflineController itself is the production
// gateway under test.
class FakeInputSource final : public application::LaunchInputSource {
public:
    enum class Behavior {
        FailImmediately,
        Block,
    };

    void load(const QString&, Completion complete) override
    {
        ++loads;
        if (behavior == Behavior::FailImmediately && complete) {
            complete(application::PreparationError{
                application::PreparationErrorCode::InvalidInput, {}, {}});
        }
    }

    Behavior behavior = Behavior::FailImmediately;
    int loads = 0;
};

class FakeJavaProbe final : public application::JavaRuntimeProbe {
public:
    void probe(const QString&, const std::filesystem::path&, Completion) override
    {
        ++probes;
    }
    void cancel(const QString&) override {}
    int probes = 0;
};

class FakeGameProcess final : public application::GameProcess {
public:
    void start(const QString&, const core::LaunchPlan&,
               const application::OfflineLaunch&, Observer) override
    {
        ++starts;
    }
    int starts = 0;
};

class FakeContentSource final : public application::ContentSource {
public:
    struct SearchCall {
        QString operationId;
        core::ContentQuery query;
    };
    struct VersionsCall {
        QString operationId;
        QString projectId;
        core::ContentQuery query;
    };

    void search(const QString& operationId, const core::ContentQuery& query,
                std::function<void(application::ContentResult<core::ContentPage>)> complete) override
    {
        searches.push_back({operationId, query});
        if (searchError.has_value()) {
            complete(searchError.value());
            return;
        }
        if (!autoCompleteSearch) {
            pendingSearch = std::move(complete);
            return;
        }
        core::ContentPage page;
        page.offset = query.offset;
        page.limit = query.limit;
        page.total = 45;
        for (int i = 0; i < 20; ++i) {
            const int number = query.offset + i;
            core::ContentProject project;
            project.id = "proj" + std::to_string(1000 + number);
            project.slug = project.id;
            project.title = "Project " + std::to_string(number);
            project.description = "Description for project " + std::to_string(number);
            project.downloads = 1000 + static_cast<std::uint64_t>(number);
            page.projects.push_back(project);
        }
        complete(page);
    }

    void versions(const QString& operationId, const std::string& projectId,
                  const core::ContentQuery& query,
                  std::function<void(application::ContentResult<std::vector<core::ContentVersion>>)> complete) override
    {
        versionsCalls.push_back({operationId, QString::fromStdString(projectId), query});
        if (versionsError.has_value()) {
            complete(versionsError.value());
            return;
        }
        if (!autoCompleteVersions) {
            pendingVersions = std::move(complete);
            return;
        }
        complete(versionResults);
    }

    void cancel(const QString&) override { ++cancelCalls; }

    std::vector<SearchCall> searches;
    std::vector<VersionsCall> versionsCalls;
    std::vector<core::ContentVersion> versionResults;
    std::optional<application::ContentError> searchError;
    std::optional<application::ContentError> versionsError;
    bool autoCompleteSearch = true;
    bool autoCompleteVersions = true;
    int cancelCalls = 0;
    std::function<void(application::ContentResult<core::ContentPage>)> pendingSearch;
    std::function<void(application::ContentResult<std::vector<core::ContentVersion>>)> pendingVersions;
};

class FakeContentDownloadStore final : public application::ContentDownloadStore {
public:
    struct AcquireCall {
        QString operationId;
        application::ContentDownloadRequest request;
    };

    void acquire(const QString& operationId,
                 const application::ContentDownloadRequest& request,
                 std::function<void(application::ContentResult<application::ContentDownloadResult>)> complete) override
    {
        calls.push_back({operationId, request});
        if (autoComplete) {
            complete(application::ContentDownloadResult{autoPath, reused});
            return;
        }
        pending = std::move(complete);
    }

    void cancel(const QString&) override
    {
        ++cancelCalls;
        if (!pending) {
            return;
        }
        auto complete = std::move(pending);
        pending = {};
        QTimer::singleShot(0, [complete = std::move(complete)]() mutable {
            complete(application::ContentError::Cancelled);
        });
    }

    void completeDownload(const QString& path, bool wasReused = false)
    {
        auto complete = std::move(pending);
        pending = {};
        complete(application::ContentDownloadResult{path, wasReused});
    }

    std::vector<AcquireCall> calls;
    bool autoComplete = false;
    QString autoPath;
    bool reused = false;
    int cancelCalls = 0;
    std::function<void(application::ContentResult<application::ContentDownloadResult>)> pending;
};

application::InstalledInstance bundledModpackInstance()
{
    return {QStringLiteral("vanilla-262"), QStringLiteral("原版 26.2"),
            QStringLiteral("26.2"), QStringLiteral("D:\\HustMCL\\data"),
            QStringLiteral("26.2"), QStringLiteral("D:\\HustMCL\\runtime\\bin\\java.exe")};
}

class FakeModpackWorkspace final : public application::ModpackWorkspace {
public:
    struct InspectCall {
        QString taskId;
        QString archive;
    };

    void inspect(const QString& taskId, const QString& archive,
                 std::function<void(application::ImportResult<core::Modpack>)> complete) override
    {
        inspectCalls.push_back({taskId, archive});
        pendingInspect = std::move(complete);
    }

    void prepareGame(const QString&, Progress progress, Done complete) override
    {
        pendingPrepare = std::move(complete);
        prepareProgress = std::move(progress);
    }

    void installFiles(const QString&, Progress progress, Done complete) override
    {
        pendingInstall = std::move(complete);
        installProgress = std::move(progress);
    }

    void commit(const QString&, std::function<void(application::ImportResult<application::InstalledInstance>)> complete) override
    {
        pendingCommit = std::move(complete);
    }

    void cancel(const QString&) override { ++cancelCalls; }
    void release(const QString&) override { ++releaseCalls; }

    void listInstalled(std::function<void(application::ImportResult<std::vector<application::InstalledInstance>>)> complete) override
    {
        ++listCalls;
        complete(installed);
    }

    void completeInspect(const core::Modpack& pack)
    {
        auto complete = std::move(pendingInspect);
        complete(pack);
    }

    void failInspect(application::ImportError error, const QString& context = {})
    {
        auto complete = std::move(pendingInspect);
        complete(application::ImportFailure{error, context});
    }

    void completePrepare()
    {
        auto complete = std::move(pendingPrepare);
        complete(std::monostate{});
    }

    void failPrepare(application::ImportError error, const QString& context = {})
    {
        auto complete = std::move(pendingPrepare);
        complete(application::ImportFailure{error, context});
    }

    void completeInstall()
    {
        auto complete = std::move(pendingInstall);
        complete(std::monostate{});
    }

    void failInstall(application::ImportError error, const QString& context = {})
    {
        auto complete = std::move(pendingInstall);
        complete(application::ImportFailure{error, context});
    }

    void completeCommit(const application::InstalledInstance& instance)
    {
        auto complete = std::move(pendingCommit);
        complete(instance);
    }

    std::vector<InspectCall> inspectCalls;
    std::vector<application::InstalledInstance> installed;
    std::function<void(application::ImportResult<core::Modpack>)> pendingInspect;
    Done pendingPrepare;
    Done pendingInstall;
    std::function<void(application::ImportResult<application::InstalledInstance>)> pendingCommit;
    Progress prepareProgress;
    Progress installProgress;
    int listCalls = 0;
    int cancelCalls = 0;
    int releaseCalls = 0;
};

class UiGateway final : public application::FrontendGateway {
public:
    application::HomeSnapshot homeSnapshot() const override { return home; }
    application::InstallSnapshot installSnapshot() const override { return install; }
    void requestPrimaryAction() override { ++primaryRequests; }
    void requestCancel(const QString& taskId) override { lastCancelId = taskId; }
    void requestRetry(const QString& taskId) override
    {
        ++retryRequests;
        lastRetryId = taskId;
    }
    bool setPlayerName(const QString& name) override
    {
        lastName = name;
        if (!acceptName) {
            return false;
        }
        home.account.displayName = name;
        emit homeChanged(home);
        return true;
    }
    void publish()
    {
        emit homeChanged(home);
        emit installChanged(install);
    }

    application::HomeSnapshot home;
    application::InstallSnapshot install;
    QString lastName;
    QString lastCancelId;
    QString lastRetryId;
    bool acceptName = true;
    int primaryRequests = 0;
    int retryRequests = 0;
};

application::HomeSnapshot realHomeSnapshot()
{
    application::HomeSnapshot home;
    home.offlineMode = true;
    home.greeting = QStringLiteral("欢迎回到 HustMCL");
    home.timeLabel = QStringLiteral("2026.10.23");
    home.account = {application::AccountState::Offline, QStringLiteral("HustPlayer"),
                    QStringLiteral("本地离线身份")};
    home.instance = {QStringLiteral("26.2"), QStringLiteral("我的原版世界"),
                     QStringLiteral("26.2"), QStringLiteral("Java 原版 · 离线单人"),
                     application::InstanceState::Unchecked};
    home.environment = {application::JavaState::Unchecked, QStringLiteral("启动前检测"),
                        QStringLiteral("x64"), QStringLiteral("随包运行时")};
    home.primaryAction = application::PrimaryAction::Launch;
    home.primaryEnabled = true;
    home.primaryLabel = QStringLiteral("启动游戏");
    home.primaryReason = QStringLiteral("启动前将校验本地文件并检测 Java。");
    home.statusTitle = QStringLiteral("从这里开启世界");
    home.dataPath = QStringLiteral("D:\\HustMCL\\data");
    home.launch = application::LaunchSummary{2048, true};
    return home;
}

core::ContentVersion testContentVersion(const std::string& projectId)
{
    core::ContentVersion version;
    version.id = "vers0001";
    version.projectId = projectId;
    version.name = "Sodium 1.0";
    version.number = "1.0.0";
    version.releaseType = "release";
    version.gameVersions = {"1.21.1"};
    version.loaders = {"fabric"};
    core::ContentFile file;
    file.name = "sodium.jar";
    file.primary = true;
    file.download.url = "https://cdn.modrinth.com/data/proj1000/versions/vers0001/sodium.jar";
    file.download.sha1 = "0123456789abcdef0123456789abcdef01234567";
    file.download.size = 1024;
    version.files.push_back(file);
    version.dependencies.push_back({"fabr0001", "fabr0002", "required"});
    return version;
}

} // namespace

class TestUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void navigationSwitchesPages();
    void primaryActionComesFromSnapshot();
    void unknownTotalHasNoPercentage();
    void knownTotalShowsProgress();
    void progressFormattingIsSaturated();
    void progressWidgetSaturatesAndRejectsNonPositiveTotals();
    void stageFailureTextUsesCurrentPhase();
    void cancelRequiresApplicationConfirmation();
    void committingCannotBeCancelled();
    void failureAndRetryReplaceTask();
    void staleCancellationDoesNotReplaceNewTask();
    void longPathElidesAndRepeatedRenderKeepsWidgetCount();
    void noDecorationHidesArtwork();
    void settingsAndLogsAreDisabled();
    void realModeUsesFivePagesAndOfflineIdentity();
    void realPrimaryLabelAndStatusTitleOverride();
    void realOfflineControllerFailureShowsStableCode();
    void realSettingsDelegatesValidationToController();
    void realCloseDuringSessionKeepsOwnerAlive();
    void realCloseWithoutSessionRequestsQuit();
    void logsPageExplainsMissingLogAndEmitsFolderIntent();
    void narrowRealWindowKeepsScaffoldScrollable();
    void uncheckedStatesUsePendingCopy();
    void checksPageShowsReadyAndRetryIntent();
    void mainWindowForwardsChecksRetryToGateway();
    void missingFolderShowsFriendlyNotice();
    void marketEntryRequiresController();
    void marketPageSearchesPaginatesAndDownloads();
    void marketPageCancelsDownloadAndShowsErrors();
    void homeImportWithoutControllerIsHidden();
    void homeImportRejectsMultipleAndRemoteDrops();
    void homeImportShowsPhasesProgressAndCommitLock();
    void homeImportCancelsRetriesAndShowsError();
    void homeInstanceSelectionAndBusyLocks();
    void homeModpackControllerReplacementIgnoresOldController();
    void homeImportAcceptsNativeQtDropEvents();
    void homeImportRendersPackAndErrorContextAsPlainText();
    void releaseArtworkMatchesBuildMode();
    void emptyModpackInstanceListIsExplained();
};

void TestUi::initTestCase()
{
    Q_INIT_RESOURCE(resources);
    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app);
    auto* style = QStyleFactory::create(QStringLiteral("Fusion"));
    QVERIFY(style);
    app->setStyle(style);
    app->setFont(hustmcl::ui::theme::baseFont());
    app->setStyleSheet(hustmcl::ui::theme::styleSheet());
    // Mirrors the real launcher: a hidden window may still own a running game.
    app->setQuitOnLastWindowClosed(false);
}

void TestUi::navigationSwitchesPages()
{
    hustmcl::app::DemoController controller;
    hustmcl::app::MainWindow window(&controller, hustmcl::app::MainWindow::Mode::Demo);
    window.show();
    QCoreApplication::processEvents();

    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Home);
    auto* nav = window.findChild<QListWidget*>(QStringLiteral("Navigation"));
    QVERIFY(nav);

    nav->setCurrentRow(1);
    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Install);
    QCOMPARE(window.installPage()->isVisible(), true);

    auto* actionBar = window.homePage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(actionBar);
    nav->setCurrentRow(0);
    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Home);
    actionBar->secondaryButton()->click();
    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Install);
}

void TestUi::primaryActionComesFromSnapshot()
{
    hustmcl::ui::HomePage page;
    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QSignalSpy spy(&page, &hustmcl::ui::HomePage::primaryActionRequested);

    application::HomeSnapshot snapshot;
    snapshot.greeting = QStringLiteral("下午好，林知远");
    snapshot.primaryAction = application::PrimaryAction::Launch;
    snapshot.primaryEnabled = true;
    snapshot.primaryReason = QStringLiteral("账号、Java 与实例均已就绪");
    page.setSnapshot(snapshot);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("启动游戏"));
    QVERIFY(bar->primaryButton()->isEnabled());
    bar->primaryButton()->click();
    QCOMPARE(spy.count(), 1);

    snapshot.primaryAction = application::PrimaryAction::SignIn;
    snapshot.primaryEnabled = false;
    snapshot.primaryReason = QStringLiteral("需要重新登录");
    page.setSnapshot(snapshot);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("登录"));
    QVERIFY(!bar->primaryButton()->isEnabled());
}

void TestUi::unknownTotalHasNoPercentage()
{
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();

    application::InstallSnapshot snapshot;
    snapshot.instanceDisplayName = QStringLiteral("校园生存");
    snapshot.versionLabel = QStringLiteral("1.21.4");
    snapshot.task.taskId = QStringLiteral("task-unknown");
    snapshot.task.phase = application::TaskPhase::Downloading;
    snapshot.task.state = application::TaskState::Running;
    snapshot.task.completedBytes = 128 * 1024 * 1024;
    snapshot.task.totalBytes = std::nullopt;
    snapshot.task.cancelAvailable = true;
    snapshot.task.phaseDetail = QStringLiteral("正在下载游戏资源");
    page.setSnapshot(snapshot);
    QCoreApplication::processEvents();

    const QString joined = joinedLabelText(page);
    QVERIFY(joined.contains(QStringLiteral("总量未知")));
    QVERIFY(!joined.contains(QLatin1Char('%')));
    for (QProgressBar* bar : page.findChildren<QProgressBar*>()) {
        QVERIFY(!bar->isVisible());
    }
}

void TestUi::knownTotalShowsProgress()
{
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();

    application::InstallSnapshot snapshot;
    snapshot.instanceDisplayName = QStringLiteral("校园生存");
    snapshot.versionLabel = QStringLiteral("1.21.4");
    snapshot.task.taskId = QStringLiteral("task-known");
    snapshot.task.phase = application::TaskPhase::Downloading;
    snapshot.task.state = application::TaskState::Running;
    snapshot.task.completedBytes = 320LL * 1024LL * 1024LL;
    snapshot.task.totalBytes = 1228LL * 1024LL * 1024LL;
    snapshot.task.cancelAvailable = true;
    page.setSnapshot(snapshot);
    QCoreApplication::processEvents();

    bool visibleProgress = false;
    for (QProgressBar* bar : page.findChildren<QProgressBar*>()) {
        if (bar->isVisible()) {
            visibleProgress = true;
            QVERIFY(bar->value() > 0);
        }
    }
    QVERIFY(visibleProgress);
    QVERIFY(joinedLabelText(page).contains(QLatin1Char('%')));
}

void TestUi::progressFormattingIsSaturated()
{
    QCOMPARE(hustmcl::ui::copy::formatPercent(134217728, 1), QStringLiteral("100%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(std::numeric_limits<qint64>::max(), 1),
             QStringLiteral("100%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(1, 3), QStringLiteral("33%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(2, 3), QStringLiteral("67%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(10, 100), QStringLiteral("10%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(-5, 100), QStringLiteral("0%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(0, 100), QStringLiteral("0%"));
    QCOMPARE(hustmcl::ui::copy::formatPercent(10, 0), QString());
    QCOMPARE(hustmcl::ui::copy::formatPercent(10, -1), QString());

    QVERIFY(hustmcl::ui::copy::progressFraction(134217728, 1) == 1.0);
    QVERIFY(hustmcl::ui::copy::progressFraction(-5, 100) == 0.0);
    QVERIFY(hustmcl::ui::copy::progressFraction(10, 0) == 0.0);
    QVERIFY(qAbs(hustmcl::ui::copy::progressFraction(1, 4) - 0.25) < 1e-12);
}

void TestUi::progressWidgetSaturatesAndRejectsNonPositiveTotals()
{
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();

    page.setSnapshot(downloadingSnapshot(50, 200));
    QCoreApplication::processEvents();
    auto* bar = visibleProgressBar(page);
    QVERIFY(bar);
    QCOMPARE(bar->value(), 250);
    QVERIFY(visibleLabelText(page).contains(QStringLiteral("25%")));

    page.setSnapshot(downloadingSnapshot(134217728, 1));
    QCoreApplication::processEvents();
    bar = visibleProgressBar(page);
    QVERIFY(bar);
    QCOMPARE(bar->value(), 1000);
    QVERIFY(visibleLabelText(page).contains(QStringLiteral("100%")));

    page.setSnapshot(downloadingSnapshot(-5, 100));
    QCoreApplication::processEvents();
    bar = visibleProgressBar(page);
    QVERIFY(bar);
    QCOMPARE(bar->value(), 0);
    const QString negativeText = visibleLabelText(page);
    QVERIFY(negativeText.contains(QStringLiteral("0%")));
    QVERIFY(!negativeText.contains(QStringLiteral("-5")));

    for (const qint64 total : {qint64(0), qint64(-1)}) {
        page.setSnapshot(downloadingSnapshot(5, total));
        QCoreApplication::processEvents();
        QVERIFY(!visibleProgressBar(page));
        const QString unknownText = visibleLabelText(page);
        QVERIFY(unknownText.contains(QStringLiteral("总量未知")));
        QVERIFY(!unknownText.contains(QLatin1Char('%')));
    }
}

void TestUi::stageFailureTextUsesCurrentPhase()
{
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();

    application::InstallSnapshot snapshot = downloadingSnapshot(0, std::nullopt);
    snapshot.task.state = application::TaskState::Failed;
    const auto setPhase = [&page, &snapshot](application::TaskPhase phase) {
        snapshot.task.phase = phase;
        page.setSnapshot(snapshot);
        QCoreApplication::processEvents();
    };

    setPhase(application::TaskPhase::PreparingEnvironment);
    auto* preparingSub = stageSub(page, 2);
    QVERIFY(preparingSub);
    QCOMPARE(preparingSub->text(), QStringLiteral("准备环境失败"));
    auto* headerMeta = page.findChild<QLabel*>(QStringLiteral("HeaderMeta"));
    QVERIFY(headerMeta);
    QCOMPARE(headerMeta->text(), QStringLiteral("阶段 3 / 6"));

    setPhase(application::TaskPhase::Verifying);
    auto* verifyingSub = stageSub(page, 4);
    QVERIFY(verifyingSub);
    QCOMPARE(verifyingSub->text(), QStringLiteral("校验失败"));
    QCOMPARE(headerMeta->text(), QStringLiteral("阶段 5 / 6"));

    setPhase(application::TaskPhase::Downloading);
    auto* downloadingSub = stageSub(page, 3);
    QVERIFY(downloadingSub);
    QCOMPARE(downloadingSub->text(), QStringLiteral("下载失败"));
    QCOMPARE(headerMeta->text(), QStringLiteral("阶段 4 / 6"));

    setPhase(application::TaskPhase::Committing);
    auto* committingSub = stageSub(page, 5);
    QVERIFY(committingSub);
    QCOMPARE(committingSub->text(), QStringLiteral("提交失败"));
    QCOMPARE(headerMeta->text(), QStringLiteral("阶段 6 / 6"));

    QCOMPARE(hustmcl::ui::installStageDefinitions().at(2).title, QStringLiteral("准备环境"));
    QCOMPARE(hustmcl::ui::installStageNumber(application::TaskPhase::PreparingEnvironment), 3);
}

void TestUi::cancelRequiresApplicationConfirmation()
{
    hustmcl::app::DemoController controller;
    controller.setScenario(hustmcl::app::DemoScenario::InstallUnknown);
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    page.setSnapshot(controller.installSnapshot());
    QObject::connect(&page, &hustmcl::ui::InstallPage::cancelRequested,
                     &controller, &hustmcl::app::DemoController::requestCancel);
    QObject::connect(&controller, &application::FrontendGateway::installChanged,
                     &page, &hustmcl::ui::InstallPage::setSnapshot);

    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    const QString taskId = controller.installSnapshot().task.taskId;
    QVERIFY(bar->secondaryButton()->isEnabled());
    bar->secondaryButton()->click();
    QCoreApplication::processEvents();

    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Cancelling);
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("正在取消")));
    QVERIFY(!bar->secondaryButton()->isEnabled());

    controller.finishCancellation(taskId);
    QCoreApplication::processEvents();
    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Cancelled);
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("已取消")));
}

void TestUi::committingCannotBeCancelled()
{
    hustmcl::app::DemoController controller;
    controller.setScenario(hustmcl::app::DemoScenario::InstallCommitting);
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    page.setSnapshot(controller.installSnapshot());
    QCoreApplication::processEvents();

    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QVERIFY(bar->secondaryButton()->isVisible());
    QVERIFY(!bar->secondaryButton()->isEnabled());
    QCOMPARE(bar->secondaryButton()->text(), QStringLiteral("取消不可用"));
    QVERIFY(bar->secondaryButton()->toolTip().contains(QStringLiteral("正在完成安装")));
}

void TestUi::failureAndRetryReplaceTask()
{
    hustmcl::app::DemoController controller;
    controller.setScenario(hustmcl::app::DemoScenario::InstallFailed);
    const QString oldId = controller.installSnapshot().task.taskId;
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    page.setSnapshot(controller.installSnapshot());
    QObject::connect(&page, &hustmcl::ui::InstallPage::retryRequested,
                     &controller, &hustmcl::app::DemoController::requestRetry);
    QObject::connect(&controller, &application::FrontendGateway::installChanged,
                     &page, &hustmcl::ui::InstallPage::setSnapshot);

    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("重试"));
    QVERIFY(bar->primaryButton()->isEnabled());
    bar->primaryButton()->click();
    QCoreApplication::processEvents();

    QVERIFY(controller.installSnapshot().task.taskId != oldId);
    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Running);
    controller.finishCancellation(oldId);
    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Running);
}

void TestUi::staleCancellationDoesNotReplaceNewTask()
{
    hustmcl::app::DemoController controller;
    controller.setScenario(hustmcl::app::DemoScenario::InstallUnknown);
    const QString oldId = controller.installSnapshot().task.taskId;
    controller.requestCancel(oldId);
    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Cancelling);

    controller.setScenario(hustmcl::app::DemoScenario::InstallUnknown);
    const QString newId = controller.installSnapshot().task.taskId;
    QVERIFY(newId != oldId);
    controller.finishCancellation(oldId);
    QCOMPARE(controller.installSnapshot().task.taskId, newId);
    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Running);
}

void TestUi::longPathElidesAndRepeatedRenderKeepsWidgetCount()
{
    hustmcl::app::DemoController controller;
    controller.setScenario(hustmcl::app::DemoScenario::InstallLongPath);
    hustmcl::ui::InstallPage page;
    page.resize(1120, 720);
    page.show();
    page.setSnapshot(controller.installSnapshot());
    QCoreApplication::processEvents();

    auto* path = page.findChild<QLabel*>(QStringLiteral("InstallPath"));
    QVERIFY(path);
    QCOMPARE(path->toolTip(), controller.installSnapshot().targetPath);
    QVERIFY(path->text() != path->toolTip());
    QVERIFY(path->text().contains(QStringLiteral("…")));

    const auto before = page.findChildren<QWidget*>().size();
    for (int i = 0; i < 5; ++i) {
        page.setSnapshot(controller.installSnapshot());
        QCoreApplication::processEvents();
    }
    QCOMPARE(page.findChildren<QWidget*>().size(), before);

    QPixmapCache::clear();
    hustmcl::ui::HomePage home;
    home.resize(1120, 720);
    home.show();
    application::HomeSnapshot snapshot;
    snapshot.greeting = QStringLiteral("下午好，林知远");
    snapshot.primaryAction = application::PrimaryAction::Launch;
    snapshot.primaryEnabled = true;
    home.setSnapshot(snapshot);
    const QPixmap rendered = home.grab();
    QVERIFY(!rendered.isNull());
    QPixmap cached;
#ifdef HUSTMCL_PUBLIC_RELEASE
    QVERIFY(!QPixmapCache::find(QStringLiteral("hustmcl/campus/xiaoke-campus"), &cached));
#else
    QVERIFY(QPixmapCache::find(QStringLiteral("hustmcl/campus/xiaoke-campus"), &cached));
#endif
}

void TestUi::noDecorationHidesArtwork()
{
    hustmcl::app::DemoController controller;
    hustmcl::app::MainWindow window(&controller, hustmcl::app::MainWindow::Mode::Demo);
    window.setDecorationsVisible(false);
    window.setCurrentPage(hustmcl::app::MainWindow::Page::Install);
    window.show();
    QCoreApplication::processEvents();

    auto* sidebarArtwork = window.sidebar()->findChild<QWidget*>(QStringLiteral("CampusArtwork"));
    QVERIFY(sidebarArtwork);
    QVERIFY(sidebarArtwork->isHidden());
    auto* sidebarCaption = window.sidebar()->findChild<QLabel*>(QStringLiteral("SidebarCaption"));
    QVERIFY(sidebarCaption);
    QVERIFY(sidebarCaption->isHidden());

    auto* homeArtwork = window.homePage()->findChild<QWidget*>(QStringLiteral("CampusArtwork"));
    QVERIFY(homeArtwork);
    QVERIFY(homeArtwork->isHidden());
    auto* homeCaption = window.homePage()->findChild<QLabel*>(QStringLiteral("SidebarCaption"));
    QVERIFY(homeCaption);
    QVERIFY(homeCaption->isHidden());
}

void TestUi::settingsAndLogsAreDisabled()
{
    hustmcl::app::DemoController controller;
    hustmcl::app::MainWindow window(&controller, hustmcl::app::MainWindow::Mode::Demo);
    window.show();
    QCoreApplication::processEvents();
    auto* nav = window.findChild<QListWidget*>(QStringLiteral("Navigation"));
    QVERIFY(nav);
    QCOMPARE(nav->count(), 5);
    QVERIFY(!(nav->item(2)->flags() & Qt::ItemIsEnabled));
    QVERIFY(!(nav->item(3)->flags() & Qt::ItemIsEnabled));
    QVERIFY(!(nav->item(4)->flags() & Qt::ItemIsEnabled));
    auto* note = window.findChild<QLabel*>(QStringLiteral("NavNote"));
    QVERIFY(note);
    QVERIFY(note->text().contains(QStringLiteral("后续批次")));
}

void TestUi::realModeUsesFivePagesAndOfflineIdentity()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    gateway.install.task.phaseDetail = QStringLiteral("尚未检查本地文件。");
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    QCoreApplication::processEvents();

    QCOMPARE(window.mode(), hustmcl::app::MainWindow::Mode::Real);
    QCOMPARE(window.windowTitle(), QStringLiteral("HustMCL"));
    QVERIFY(window.checksPage());
    QVERIFY(!window.installPage());
    QVERIFY(window.settingsPage());
    QVERIFY(window.logsPage());
    QVERIFY(window.contentMarketPage());

    auto* nav = window.findChild<QListWidget*>(QStringLiteral("Navigation"));
    QVERIFY(nav);
    QCOMPARE(nav->count(), 5);
    QVERIFY(!(nav->item(1)->text().contains(QStringLiteral("安装游戏"))));
    QVERIFY(nav->item(1)->text().contains(QStringLiteral("实例与检查")));
    QVERIFY(nav->item(4)->text().contains(QStringLiteral("模组市场")));
    for (int row = 0; row < 4; ++row) {
        QVERIFY(nav->item(row)->flags() & Qt::ItemIsEnabled);
        QVERIFY(nav->item(row)->flags() & Qt::ItemIsSelectable);
    }
    QVERIFY(!(nav->item(4)->flags() & Qt::ItemIsEnabled));

    auto* badge = window.findChild<QLabel*>(QStringLiteral("DemoBadge"));
    QVERIFY(badge);
    QCOMPARE(badge->text(), QStringLiteral("本地离线体验"));
    auto* accountState = window.findChild<QLabel*>(QStringLiteral("AccountState"));
    QVERIFY(accountState);
    QCOMPARE(accountState->text(), QStringLiteral("本地离线"));
    QVERIFY(joinedLabelText(window).contains(QStringLiteral("离线身份仅用于本地游戏，不是正版登录。")));
    QVERIFY(!joinedLabelText(window).contains(QStringLiteral("已登录正版")));

    auto* homeBar = window.homePage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(homeBar);
    QCOMPARE(homeBar->primaryButton()->text(), QStringLiteral("启动游戏"));
    QVERIFY(homeBar->secondaryButton()->isVisible());
    QVERIFY(!homeBar->secondaryButton()->isEnabled());
    QVERIFY(homeBar->secondaryButton()->text().contains(QStringLiteral("未接入")));

    for (const auto page : {hustmcl::app::MainWindow::Page::InstanceChecks,
                            hustmcl::app::MainWindow::Page::Settings,
                            hustmcl::app::MainWindow::Page::Logs}) {
        window.setCurrentPage(page);
        QCoreApplication::processEvents();
        QCOMPARE(window.currentPage(), page);
    }
}

void TestUi::realPrimaryLabelAndStatusTitleOverride()
{
    hustmcl::ui::HomePage page;
    application::HomeSnapshot snapshot = realHomeSnapshot();
    snapshot.statusTitle = QStringLiteral("检查游戏文件");
    snapshot.primaryLabel = QStringLiteral("校验中…");
    snapshot.primaryEnabled = false;
    snapshot.primaryReason = QStringLiteral("正在校验本地原版文件。");
    page.setSnapshot(snapshot);

    auto* title = page.findChild<QLabel*>(QStringLiteral("ActionTitle"));
    QVERIFY(title);
    QCOMPARE(title->text(), QStringLiteral("检查游戏文件"));
    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("校验中…"));
    QVERIFY(!bar->primaryButton()->isEnabled());
    QCOMPARE(hustmcl::ui::copy::accountStateText(application::AccountState::Offline),
             QStringLiteral("本地离线"));
}

void TestUi::realOfflineControllerFailureShowsStableCode()
{
    FakeInputSource source;
    FakeJavaProbe probe;
    application::LaunchPreparation preparation(source, probe);
    FakeGameProcess process;
    application::OfflineController controller(
        preparation, process, QStringLiteral("D:\\HustMCL\\data"), QStringLiteral("26.2"));
    QVERIFY(controller.setPlayerName(QStringLiteral("HustPlayer")));

    hustmcl::app::MainWindow window(&controller, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    QCoreApplication::processEvents();

    auto* bar = window.homePage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QVERIFY(bar->primaryButton()->isEnabled());
    bar->primaryButton()->click();
    QCoreApplication::processEvents();

    QCOMPARE(source.loads, 1);
    QVERIFY(!controller.homeSnapshot().sessionActive);
    QVERIFY(controller.installSnapshot().task.error.has_value());
    QCOMPARE(controller.installSnapshot().task.error->code,
             application::TaskErrorCode::Integrity);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("重试启动"));
    QVERIFY(bar->primaryButton()->isEnabled());

    window.setCurrentPage(hustmcl::app::MainWindow::Page::InstanceChecks);
    QCoreApplication::processEvents();
    const QString checksText = joinedLabelText(*window.checksPage());
    QVERIFY(checksText.contains(QStringLiteral("INTEGRITY")));
    QVERIFY(checksText.contains(QStringLiteral("本地启动准备未通过")));
    QVERIFY(!checksText.contains(QStringLiteral("下载")));
    QVERIFY(checksText.contains(QStringLiteral("文件校验")));

    window.setCurrentPage(hustmcl::app::MainWindow::Page::Logs);
    QCoreApplication::processEvents();
    const QString logsText = joinedLabelText(*window.logsPage());
    QVERIFY(logsText.contains(QStringLiteral("最近错误")));
    QVERIFY(logsText.contains(QStringLiteral("INTEGRITY")));
    QVERIFY(logsText.contains(QStringLiteral("可重试：是")));
}

void TestUi::realSettingsDelegatesValidationToController()
{
    FakeInputSource source;
    FakeJavaProbe probe;
    application::LaunchPreparation preparation(source, probe);
    FakeGameProcess process;
    application::OfflineController controller(
        preparation, process, QStringLiteral("D:\\HustMCL\\data"), QStringLiteral("26.2"));
    QVERIFY(controller.setPlayerName(QStringLiteral("HustPlayer")));

    hustmcl::app::MainWindow window(&controller, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    window.setCurrentPage(hustmcl::app::MainWindow::Page::Settings);
    QCoreApplication::processEvents();

    auto* input = window.settingsPage()->findChild<QLineEdit*>(QStringLiteral("PlayerNameInput"));
    QVERIFY(input);
    auto* bar = window.settingsPage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QVERIFY(bar->primaryButton()->isEnabled());

    input->setText(QStringLiteral("Campus_Player"));
    bar->primaryButton()->click();
    QCoreApplication::processEvents();
    QCOMPARE(controller.homeSnapshot().account.displayName, QStringLiteral("Campus_Player"));
    QCOMPARE(input->text(), QStringLiteral("Campus_Player"));
    auto* success = window.settingsPage()->findChild<QLabel*>(QStringLiteral("FieldSuccess"));
    QVERIFY(success);
    QVERIFY(success->isVisible());
    QVERIFY(success->text().contains(QStringLiteral("本次启动器打开期间生效")));

    auto* error = window.settingsPage()->findChild<QLabel*>(QStringLiteral("FieldError"));
    QVERIFY(error);
    input->setText(QStringLiteral("林知远"));
    bar->primaryButton()->click();
    QCoreApplication::processEvents();
    QVERIFY(error->isVisible());
    QVERIFY(error->text().contains(QStringLiteral("离线名无效")));
    QCOMPARE(controller.homeSnapshot().account.displayName, QStringLiteral("Campus_Player"));

    const QString settingsText = joinedLabelText(*window.settingsPage());
    QVERIFY(settingsText.contains(QStringLiteral("2048 MB")));
    QVERIFY(settingsText.contains(QStringLiteral("26.2")));
    QVERIFY(settingsText.contains(QStringLiteral("D:\\HustMCL\\data")));
    QVERIFY(settingsText.contains(QStringLiteral("未接入")));
}

void TestUi::realCloseDuringSessionKeepsOwnerAlive()
{
    FakeInputSource source;
    source.behavior = FakeInputSource::Behavior::Block;
    FakeJavaProbe probe;
    application::LaunchPreparation preparation(source, probe);
    FakeGameProcess process;
    application::OfflineController controller(
        preparation, process, QStringLiteral("D:\\HustMCL\\data"), QStringLiteral("26.2"));
    QVERIFY(controller.setPlayerName(QStringLiteral("HustPlayer")));

    hustmcl::app::MainWindow window(&controller, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    QCoreApplication::processEvents();
    QSignalSpy quitSpy(&window, &hustmcl::app::MainWindow::quitRequested);

    controller.requestPrimaryAction();
    QCoreApplication::processEvents();
    QVERIFY(controller.homeSnapshot().sessionActive);
    QVERIFY(controller.installSnapshot().task.cancelAvailable);
    const QString taskId = controller.installSnapshot().task.taskId;
    QVERIFY(!taskId.isEmpty());

    window.setCurrentPage(hustmcl::app::MainWindow::Page::Settings);
    QCoreApplication::processEvents();
    auto* input = window.settingsPage()->findChild<QLineEdit*>(QStringLiteral("PlayerNameInput"));
    QVERIFY(input);
    QVERIFY(!input->isEnabled());
    auto* settingsBar = window.settingsPage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(settingsBar);
    QVERIFY(!settingsBar->primaryButton()->isEnabled());

    window.setCurrentPage(hustmcl::app::MainWindow::Page::Home);
    QCoreApplication::processEvents();
    window.close();
    QCoreApplication::processEvents();
    QVERIFY(!window.isVisible());
    QCOMPARE(quitSpy.count(), 0);
    QVERIFY(window.homePage() != nullptr);
    QVERIFY(controller.homeSnapshot().sessionActive);

    controller.requestCancel(taskId);
    QCoreApplication::processEvents();
    QVERIFY(!controller.homeSnapshot().sessionActive);
    QCOMPARE(quitSpy.count(), 1);
    QVERIFY(window.homePage() != nullptr);
}

void TestUi::realCloseWithoutSessionRequestsQuit()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    QCoreApplication::processEvents();
    QSignalSpy quitSpy(&window, &hustmcl::app::MainWindow::quitRequested);

    window.close();
    QCoreApplication::processEvents();
    QCOMPARE(quitSpy.count(), 1);
}

void TestUi::logsPageExplainsMissingLogAndEmitsFolderIntent()
{
    hustmcl::ui::LogsPage page;
    page.resize(840, 600);
    page.show();
    application::HomeSnapshot home = realHomeSnapshot();
    application::TaskSnapshot task;
    task.phase = application::TaskPhase::Verifying;
    task.state = application::TaskState::Failed;
    task.error = application::TaskError{
        application::TaskErrorCode::Integrity,
        QStringLiteral("本地启动准备未通过"),
        QStringLiteral("请确认版本、assets 和 libraries 目录完整。"),
        true,
    };
    page.setSnapshot(home, task);
    QCoreApplication::processEvents();

    const QString initial = joinedLabelText(page);
    QVERIFY(initial.contains(QStringLiteral("还没有运行日志")));
    QVERIFY(initial.contains(QStringLiteral("错误码 INTEGRITY")));
    QVERIFY(initial.contains(QStringLiteral("在此查看本次启动状态")));
    QVERIFY(!initial.contains(QStringLiteral("应用快照")));
    QVERIFY(!initial.contains(QStringLiteral("本页")));

    auto* openLogs = buttonWithText(page, QStringLiteral("打开日志文件夹"));
    auto* openSaves = buttonWithText(page, QStringLiteral("打开游戏存档文件夹"));
    QVERIFY(openLogs);
    QVERIFY(openSaves);
    QVERIFY(!openLogs->isEnabled());
    QVERIFY(openSaves->isEnabled());

    QSignalSpy logsSpy(&page, &hustmcl::ui::LogsPage::openLogFolderRequested);
    QSignalSpy savesSpy(&page, &hustmcl::ui::LogsPage::openSavesFolderRequested);
    openSaves->click();
    QCOMPARE(savesSpy.count(), 1);
    QCOMPARE(logsSpy.count(), 0);

    home.logPath = QStringLiteral("D:\\HustMCL\\data\\launcher-logs\\run-1");
    page.setSnapshot(home, task);
    QCoreApplication::processEvents();
    QVERIFY(openLogs->isEnabled());
    auto* logPath = page.findChild<QLabel*>(QStringLiteral("PathValue"));
    QVERIFY(logPath);
    QCOMPARE(openLogs->toolTip(), QStringLiteral("打开包含本次运行日志的文件夹。"));
    openLogs->click();
    QCOMPARE(logsSpy.count(), 1);
}

void TestUi::narrowRealWindowKeepsScaffoldScrollable()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    QCOMPARE(window.minimumSize(), QSize(840, 460));
    window.resize(840, 460);
    window.show();
    QCoreApplication::processEvents();

    const auto scrollAreas = window.findChildren<QScrollArea*>(QStringLiteral("DeskScroll"));
    QCOMPARE(scrollAreas.size(), 5);
    for (hustmcl::app::MainWindow::Page page : {
             hustmcl::app::MainWindow::Page::Home,
             hustmcl::app::MainWindow::Page::InstanceChecks,
             hustmcl::app::MainWindow::Page::Settings,
             hustmcl::app::MainWindow::Page::Logs,
         }) {
        window.setCurrentPage(page);
        QCoreApplication::processEvents();
        QCOMPARE(window.currentPage(), page);
        QVERIFY(!window.grab().isNull());
    }

    window.resize(1120, 720);
    QCoreApplication::processEvents();
    QVERIFY(window.size().width() >= 840);
    QVERIFY(window.size().height() >= 460);
}

void TestUi::uncheckedStatesUsePendingCopy()
{
    QCOMPARE(hustmcl::ui::copy::instanceStateText(application::InstanceState::Unchecked),
             QStringLiteral("待检查"));
    QCOMPARE(hustmcl::ui::copy::javaStateText(application::JavaState::Unchecked),
             QStringLiteral("待检测"));

    const application::HomeSnapshot home = realHomeSnapshot();
    hustmcl::ui::HomePage page;
    page.setSnapshot(home);
    const QString homeText = joinedLabelText(page);
    QVERIFY(homeText.contains(QStringLiteral("待检查")));
    QVERIFY(homeText.contains(QStringLiteral("待检测")));
    QVERIFY(!homeText.contains(QStringLiteral("不完整")));

    hustmcl::ui::ChecksPage checks;
    checks.setSnapshot(home, {});
    const QString checksText = joinedLabelText(checks);
    QVERIFY(checksText.contains(QStringLiteral("待检查")));
    QVERIFY(checksText.contains(QStringLiteral("待检测")));
    QVERIFY(!checksText.contains(QStringLiteral("不完整")));
}

void TestUi::checksPageShowsReadyAndRetryIntent()
{
    application::HomeSnapshot home = realHomeSnapshot();
    home.instance.state = application::InstanceState::Ready;
    application::TaskSnapshot task;
    task.taskId = QStringLiteral("task-check");
    task.phase = application::TaskPhase::PreparingEnvironment;
    task.state = application::TaskState::Running;
    task.cancelAvailable = true;

    hustmcl::ui::ChecksPage page;
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();
    page.setSnapshot(home, task);
    const QString runningText = joinedLabelText(page);
    QVERIFY(runningText.contains(QStringLiteral("已通过")));
    QVERIFY(!runningText.contains(QStringLiteral("不提供结束游戏")));
    QVERIFY(!runningText.contains(QStringLiteral("提前宣称")));

    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QVERIFY(bar->primaryButton()->isHidden());
    QVERIFY(bar->secondaryButton()->isVisible());
    QVERIFY(bar->secondaryButton()->isEnabled());

    task.phase = application::TaskPhase::Verifying;
    task.state = application::TaskState::Failed;
    task.cancelAvailable = false;
    task.retryAvailable = true;
    task.retryReason = QStringLiteral("再次检查并启动。");
    task.error = application::TaskError{
        application::TaskErrorCode::Integrity,
        QStringLiteral("本地启动准备未通过"),
        QStringLiteral("请确认版本目录完整。"),
        true,
    };
    page.setSnapshot(home, task);
    QCoreApplication::processEvents();

    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("重试启动"));
    QVERIFY(bar->primaryButton()->isVisible());
    QVERIFY(bar->primaryButton()->isEnabled());
    QSignalSpy retrySpy(&page, &hustmcl::ui::ChecksPage::retryRequested);
    bar->primaryButton()->click();
    QCOMPARE(retrySpy.count(), 1);
    QCOMPARE(retrySpy.takeFirst().at(0).toString(), QStringLiteral("task-check"));
}

void TestUi::mainWindowForwardsChecksRetryToGateway()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    gateway.install.task.taskId = QStringLiteral("task-retry");
    gateway.install.task.phase = application::TaskPhase::Verifying;
    gateway.install.task.state = application::TaskState::Failed;
    gateway.install.task.retryAvailable = true;
    gateway.install.task.error = application::TaskError{
        application::TaskErrorCode::Integrity,
        QStringLiteral("本地启动准备未通过"),
        QStringLiteral("请确认版本目录完整。"),
        true,
    };

    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    window.setCurrentPage(hustmcl::app::MainWindow::Page::InstanceChecks);
    QCoreApplication::processEvents();

    auto* bar = window.checksPage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("重试启动"));
    bar->primaryButton()->click();
    QCoreApplication::processEvents();
    QCOMPARE(gateway.retryRequests, 1);
    QCOMPARE(gateway.lastRetryId, QStringLiteral("task-retry"));
}

void TestUi::missingFolderShowsFriendlyNotice()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    gateway.home.dataPath = QStringLiteral("D:\\HustMCL\\missing-data-root-for-ui-test");
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    window.setCurrentPage(hustmcl::app::MainWindow::Page::Logs);
    QCoreApplication::processEvents();

    auto* openSaves = buttonWithText(*window.logsPage(),
                                     QStringLiteral("打开游戏存档文件夹"));
    QVERIFY(openSaves);
    QVERIFY(openSaves->isEnabled());

    QString notice;
    QTimer::singleShot(0, [&notice]() {
        auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box) {
            return;
        }
        notice = box->text();
        box->accept();
    });
    openSaves->click();
    QCOMPARE(notice, QStringLiteral("启动游戏并创建世界后，存档文件夹才会生成。"));
}

void TestUi::marketEntryRequiresController()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    QCoreApplication::processEvents();

    auto* nav = window.findChild<QListWidget*>(QStringLiteral("Navigation"));
    QVERIFY(nav);
    QCOMPARE(nav->count(), 5);
    QVERIFY(!(nav->item(4)->flags() & Qt::ItemIsEnabled));
    auto* emptyResults = window.contentMarketPage()->findChild<QListWidget*>(
        QStringLiteral("MarketResults"));
    QVERIFY(emptyResults);
    QCOMPARE(emptyResults->count(), 0);
    window.setCurrentPage(hustmcl::app::MainWindow::Page::Market);
    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Home);

    FakeContentSource source;
    FakeContentDownloadStore store;
    application::ContentController controller(source, store);
    window.setContentController(&controller);
    QVERIFY(nav->item(4)->flags() & Qt::ItemIsEnabled);
    QVERIFY(nav->item(4)->flags() & Qt::ItemIsSelectable);
    window.setCurrentPage(hustmcl::app::MainWindow::Page::Market);
    QCoreApplication::processEvents();
    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Market);
    QVERIFY(window.contentMarketPage()->isVisible());
    QCOMPARE(window.contentMarketPage()->controller(), &controller);

    window.setContentController(nullptr);
    QCOMPARE(window.currentPage(), hustmcl::app::MainWindow::Page::Home);
    QVERIFY(!(nav->item(4)->flags() & Qt::ItemIsEnabled));
}

void TestUi::marketPageSearchesPaginatesAndDownloads()
{
    FakeContentSource source;
    source.versionResults = {testContentVersion("proj1000")};
    FakeContentDownloadStore store;
    application::ContentController controller(source, store);
    hustmcl::ui::ContentMarketPage page(&controller);
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();

    auto* keyword = page.findChild<QLineEdit*>(QStringLiteral("MarketSearchInput"));
    auto* versionInput = page.findChild<QLineEdit*>(QStringLiteral("MarketVersionInput"));
    auto* loader = page.findChild<QComboBox*>(QStringLiteral("MarketLoader"));
    auto* search = page.findChild<QPushButton*>(QStringLiteral("MarketSearch"));
    auto* results = page.findChild<QListWidget*>(QStringLiteral("MarketResults"));
    auto* versions = page.findChild<QListWidget*>(QStringLiteral("MarketVersions"));
    auto* dependency = page.findChild<QLabel*>(QStringLiteral("MarketDependency"));
    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(keyword);
    QVERIFY(versionInput);
    QVERIFY(loader);
    QVERIFY(search);
    QVERIFY(results);
    QVERIFY(versions);
    QVERIFY(dependency);
    QVERIFY(bar);

    keyword->setText(QStringLiteral("sodium"));
    versionInput->setText(QStringLiteral("1.21.1"));
    loader->setCurrentIndex(loader->findData(QStringLiteral("fabric")));
    search->click();
    QCoreApplication::processEvents();

    QCOMPARE(source.searches.size(), std::size_t(1));
    QCOMPARE(QString::fromStdString(source.searches.at(0).query.text), QStringLiteral("sodium"));
    QCOMPARE(QString::fromStdString(source.searches.at(0).query.gameVersion), QStringLiteral("1.21.1"));
    QCOMPARE(QString::fromStdString(source.searches.at(0).query.loader), QStringLiteral("fabric"));
    QCOMPARE(source.searches.at(0).query.offset, 0);
    QCOMPARE(source.searches.at(0).query.limit, 20);
    QCOMPARE(results->count(), 20);
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("45")));

    auto* next = buttonWithText(page, QStringLiteral("下一页"));
    auto* previous = buttonWithText(page, QStringLiteral("上一页"));
    QVERIFY(next);
    QVERIFY(previous);
    QVERIFY(next->isEnabled());
    next->click();
    QCoreApplication::processEvents();
    QCOMPARE(source.searches.size(), std::size_t(2));
    QCOMPARE(source.searches.at(1).query.offset, 20);
    QVERIFY(previous->isEnabled());
    previous->click();
    QCoreApplication::processEvents();
    QCOMPARE(source.searches.size(), std::size_t(3));
    QCOMPARE(source.searches.at(2).query.offset, 0);

    results->setCurrentRow(0);
    QCoreApplication::processEvents();
    QCOMPARE(source.versionsCalls.size(), std::size_t(1));
    QCOMPARE(source.versionsCalls.at(0).projectId, QStringLiteral("proj1000"));
    QCOMPARE(QString::fromStdString(source.versionsCalls.at(0).query.text), QStringLiteral("sodium"));
    QCOMPARE(QString::fromStdString(source.versionsCalls.at(0).query.loader), QStringLiteral("fabric"));
    QCOMPARE(versions->count(), 1);
    const QString projectText = joinedLabelText(page);
    QVERIFY(projectText.contains(QStringLiteral("关键词「sodium」")));
    QVERIFY(projectText.contains(QStringLiteral("MC 1.21.1")));
    QVERIFY(projectText.contains(QStringLiteral("Fabric")));

    versions->setCurrentRow(0);
    QCoreApplication::processEvents();
    QVERIFY(dependency->text().contains(QStringLiteral("必需前置")));
    QVERIFY(dependency->text().contains(QStringLiteral("fabr0001")));
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("下载文件"));
    QVERIFY(bar->primaryButton()->isEnabled());
    bar->primaryButton()->click();
    QCoreApplication::processEvents();

    QCOMPARE(store.calls.size(), std::size_t(1));
    QCOMPARE(QString::fromStdString(store.calls.at(0).request.projectId), QStringLiteral("proj1000"));
    QCOMPARE(QString::fromStdString(store.calls.at(0).request.versionId), QStringLiteral("vers0001"));
    QCOMPARE(QString::fromStdString(store.calls.at(0).request.file.name), QStringLiteral("sodium.jar"));
    QVERIFY(controller.snapshot().phase == application::ContentPhase::Downloading);
    QCOMPARE(bar->secondaryButton()->text(), QStringLiteral("取消下载"));
    QVERIFY(bar->secondaryButton()->isVisible());
    QVERIFY(bar->secondaryButton()->isEnabled());
    QCOMPARE(versions->currentRow(), 0);
    QVERIFY(!joinedLabelText(page).contains(QLatin1Char('%')));

    const QString downloadedPath =
        QStringLiteral("D:\\HustMCL\\data\\content-downloads\\modrinth\\proj1000\\vers0001\\sodium.jar");
    store.completeDownload(downloadedPath, true);
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ContentPhase::Downloaded);
    QVERIFY(controller.snapshot().reused);
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("下载完成")));
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("复用")));
    QCOMPARE(bar->secondaryButton()->text(), QStringLiteral("打开下载文件夹"));
    QVERIFY(bar->secondaryButton()->isEnabled());

    QSignalSpy openSpy(&page, &hustmcl::ui::ContentMarketPage::openDownloadFolderRequested);
    bar->secondaryButton()->click();
    QCOMPARE(openSpy.count(), 1);
    QCOMPARE(openSpy.takeFirst().at(0).toString(), downloadedPath);
}

void TestUi::marketPageCancelsDownloadAndShowsErrors()
{
    FakeContentSource source;
    source.versionResults = {testContentVersion("proj1000")};
    FakeContentDownloadStore store;
    application::ContentController controller(source, store);
    hustmcl::ui::ContentMarketPage page(&controller);
    page.resize(1120, 720);
    page.show();
    QCoreApplication::processEvents();

    auto* search = page.findChild<QPushButton*>(QStringLiteral("MarketSearch"));
    auto* results = page.findChild<QListWidget*>(QStringLiteral("MarketResults"));
    auto* versions = page.findChild<QListWidget*>(QStringLiteral("MarketVersions"));
    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(search);
    QVERIFY(results);
    QVERIFY(versions);
    QVERIFY(bar);

    search->click();
    QCoreApplication::processEvents();
    results->setCurrentRow(0);
    QCoreApplication::processEvents();
    versions->setCurrentRow(0);
    QCoreApplication::processEvents();
    bar->primaryButton()->click();
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ContentPhase::Downloading);

    bar->secondaryButton()->click();
    QVERIFY(controller.snapshot().phase == application::ContentPhase::Cancelling);
    QCOMPARE(bar->secondaryButton()->text(), QStringLiteral("正在取消…"));
    QVERIFY(!bar->secondaryButton()->isEnabled());
    QTRY_VERIFY(controller.snapshot().phase == application::ContentPhase::Cancelled);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("重新下载"));
    QVERIFY(page.findChildren<QProgressBar*>().isEmpty());

    source.searchError = application::ContentError::Network;
    search->click();
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ContentPhase::Failed);
    QVERIFY(controller.snapshot().error == application::ContentError::Network);
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("NETWORK")));
    QVERIFY(joinedLabelText(page).contains(QStringLiteral("无法连接 Modrinth")));
    QVERIFY(bar->primaryButton()->isHidden());
}

void TestUi::homeImportWithoutControllerIsHidden()
{
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.show();
    QCoreApplication::processEvents();

    auto* section = window.homePage()->findChild<QWidget*>(QStringLiteral("ModpackSection"));
    auto* combo = window.homePage()->findChild<QComboBox*>(QStringLiteral("ModpackInstanceCombo"));
    auto* pick = modpackPickButton(*window.homePage());
    QVERIFY(section);
    QVERIFY(combo);
    QVERIFY(pick);
    QVERIFY(!section->isVisible());
    QVERIFY(!combo->isVisible());
    QVERIFY(!pick->isVisible());

    sendUrlDrop(*window.homePage(), {QUrl::fromLocalFile(QStringLiteral("C:/packs/demo.zip"))});
    QVERIFY(!section->isVisible());
}

void TestUi::homeImportRejectsMultipleAndRemoteDrops()
{
    FakeModpackWorkspace workspace;
    application::ModpackController controller(workspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(&controller);
    window.show();
    QCoreApplication::processEvents();

    auto* hint = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackHint"));
    QVERIFY(hint);
    QVERIFY(!hint->isVisible());
    auto* section = window.homePage()->findChild<QWidget*>(QStringLiteral("ModpackSection"));
    QVERIFY(section);
    QVERIFY(section->isVisible());
    QVERIFY(window.homePage()->acceptDrops());

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/a.zip")),
        QUrl::fromLocalFile(QStringLiteral("C:/packs/b.zip")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(0));
    QVERIFY(hint->isVisible());
    QVERIFY(hint->text().contains(QStringLiteral("一次只能导入一个")));

    sendUrlDrop(*window.homePage(), {
        QUrl(QStringLiteral("https://example.com/pack.zip")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(0));
    QVERIFY(hint->text().contains(QStringLiteral("远程链接")));

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/readme.txt")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(0));
    QVERIFY(hint->text().contains(QStringLiteral(".zip")));

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.zip")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(1));
    QCOMPARE(workspace.inspectCalls.at(0).archive,
             QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.zip")).toLocalFile());
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Inspecting);
    QVERIFY(!hint->isVisible());
}

void TestUi::homeImportShowsPhasesProgressAndCommitLock()
{
    FakeModpackWorkspace workspace;
    application::ModpackController controller(workspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(&controller);
    window.show();
    QCoreApplication::processEvents();

    auto* phase = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackPhase"));
    auto* progress = window.homePage()->findChild<QProgressBar*>(QStringLiteral("ImportProgress"));
    auto* progressText = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackProgressText"));
    auto* combo = window.homePage()->findChild<QComboBox*>(QStringLiteral("ModpackInstanceCombo"));
    auto* pick = modpackPickButton(*window.homePage());
    auto* bar = window.homePage()->findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(phase);
    QVERIFY(progress);
    QVERIFY(progressText);
    QVERIFY(combo);
    QVERIFY(pick);
    QVERIFY(bar);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("启动游戏"));
    QVERIFY(bar->primaryButton()->isEnabled());

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.zip")),
    });
    QCoreApplication::processEvents();
    QCOMPARE(phase->text(), QStringLiteral("读取整合包"));
    QVERIFY(!pick->isEnabled());
    QVERIFY(!combo->isEnabled());
    auto* cancel = buttonWithText(*window.homePage(), QStringLiteral("取消导入"));
    QVERIFY(cancel);
    QVERIFY(cancel->isVisible());
    QVERIFY(cancel->isEnabled());
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("启动游戏"));

    core::Modpack pack;
    pack.name = "Campus Pack";
    pack.version = "1.0";
    pack.minecraft = "1.21.1";
    workspace.completeInspect(pack);
    QCoreApplication::processEvents();
    QCOMPARE(phase->text(), QStringLiteral("准备游戏环境"));
    workspace.prepareProgress({1, 2, 0, 0});
    QCoreApplication::processEvents();
    QVERIFY(progressText->isVisible());
    QVERIFY(progressText->text().contains(QStringLiteral("文件 1 / 2")));
    QVERIFY(!progressText->text().contains(QLatin1Char('%')));
    QVERIFY(progress->isVisible());
    QCOMPARE(progress->value(), 500);

    workspace.completePrepare();
    QCoreApplication::processEvents();
    QCOMPARE(phase->text(), QStringLiteral("安装整合包文件"));
    workspace.installProgress({3, 10, 500, 0});
    QCoreApplication::processEvents();
    QVERIFY(progressText->text().contains(QStringLiteral("文件 3 / 10")));
    QVERIFY(progressText->text().contains(QStringLiteral("已传输 500 B")));
    QVERIFY(progressText->text().contains(QStringLiteral("总量未知")));
    QVERIFY(!progressText->text().contains(QLatin1Char('%')));
    QCOMPARE(progress->value(), 300);

    workspace.completeInstall();
    QCoreApplication::processEvents();
    QCOMPARE(phase->text(), QStringLiteral("完成安装"));
    QCOMPARE(cancel->text(), QStringLiteral("正在完成安装"));
    QVERIFY(!cancel->isEnabled());
    cancel->click();
    QCoreApplication::processEvents();
    QCOMPARE(workspace.cancelCalls, 0);
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Committing);

    const application::InstalledInstance installed{
        QStringLiteral("campus-pack"), QStringLiteral("Campus Pack"),
        QStringLiteral("1.0"), QStringLiteral("D:\\HustMCL\\data\\instances\\campus-pack"),
        QStringLiteral("1.21.1"), QStringLiteral("D:\\HustMCL\\runtime\\bin\\java.exe")};
    workspace.completeCommit(installed);
    QCoreApplication::processEvents();
    QCOMPARE(phase->text(), QStringLiteral("安装完成"));
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Succeeded);
    QVERIFY(combo->isEnabled());
    QVERIFY(combo->findData(QStringLiteral("campus-pack")) >= 0);
    QCOMPARE(combo->currentData().toString(), QStringLiteral("campus-pack"));
    QVERIFY(pick->isEnabled());
    QVERIFY(!cancel->isVisible());
    auto* retry = buttonWithText(*window.homePage(), QStringLiteral("重试导入"));
    QVERIFY(retry);
    QVERIFY(!retry->isVisible());
    QVERIFY(joinedLabelText(*window.homePage()).contains(QStringLiteral("可选客户端内容")));
    QCOMPARE(gateway.primaryRequests, 0);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("启动游戏"));
}

void TestUi::homeImportCancelsRetriesAndShowsError()
{
    FakeModpackWorkspace workspace;
    application::ModpackController controller(workspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(&controller);
    window.show();
    QCoreApplication::processEvents();

    auto* phase = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackPhase"));
    auto* errorLabel = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackError"));
    QVERIFY(phase);
    QVERIFY(errorLabel);

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/broken.zip")),
    });
    QCoreApplication::processEvents();
    auto* cancel = buttonWithText(*window.homePage(), QStringLiteral("取消导入"));
    QVERIFY(cancel);
    QVERIFY(cancel->isVisible());
    cancel->click();
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Cancelling);
    QCOMPARE(cancel->text(), QStringLiteral("正在取消…"));
    QVERIFY(!cancel->isEnabled());
    auto* retry = buttonWithText(*window.homePage(), QStringLiteral("重试导入"));
    QVERIFY(retry);
    QVERIFY(!retry->isVisible());

    workspace.failInspect(application::ImportError::Cancelled);
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Cancelled);
    QCOMPARE(phase->text(), QStringLiteral("已取消"));
    QVERIFY(!cancel->isVisible());
    QVERIFY(retry->isVisible());
    QVERIFY(retry->isEnabled());
    QCOMPARE(workspace.releaseCalls, 1);

    retry->click();
    QCoreApplication::processEvents();
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(2));
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Inspecting);

    workspace.failInspect(application::ImportError::InvalidArchive,
                          QStringLiteral("不是有效的 ZIP 文件。"));
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Failed);
    QCOMPARE(phase->text(), QStringLiteral("导入失败"));
    QVERIFY(errorLabel->isVisible());
    QVERIFY(errorLabel->text().contains(QStringLiteral("无法读取整合包")));
    QVERIFY(errorLabel->text().contains(QStringLiteral("不是有效的 ZIP 文件。")));
    QVERIFY(!errorLabel->text().contains(QStringLiteral("InvalidArchive")));
    QVERIFY(retry->isVisible());
    QVERIFY(retry->isEnabled());

    retry->click();
    QCoreApplication::processEvents();
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(3));
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Inspecting);
}

void TestUi::homeInstanceSelectionAndBusyLocks()
{
    FakeModpackWorkspace workspace;
    workspace.installed = {
        {QStringLiteral("campus-pack"), QStringLiteral("Campus Pack"),
         QStringLiteral("1.0"), QStringLiteral("D:\\HustMCL\\data\\instances\\campus-pack"),
         QStringLiteral("1.21.1"), QStringLiteral("D:\\HustMCL\\runtime\\bin\\java.exe")},
    };
    auto controller = std::make_unique<application::ModpackController>(
        workspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(controller.get());
    window.setModpackController(controller.get());
    window.show();
    QCoreApplication::processEvents();

    auto* combo = window.homePage()->findChild<QComboBox*>(QStringLiteral("ModpackInstanceCombo"));
    auto* pick = modpackPickButton(*window.homePage());
    QVERIFY(combo);
    QVERIFY(pick);
    QCOMPARE(combo->count(), 2);
    QVERIFY(combo->findData(QStringLiteral("vanilla-262")) >= 0);
    QVERIFY(combo->findData(QStringLiteral("campus-pack")) >= 0);
    QSignalSpy selectedSpy(controller.get(), &application::ModpackController::instanceSelected);
    combo->setCurrentIndex(combo->findData(QStringLiteral("campus-pack")));
    QCoreApplication::processEvents();
    QCOMPARE(controller->snapshot().selectedId, QStringLiteral("campus-pack"));
    QCOMPARE(selectedSpy.count(), 1);
    combo->setCurrentIndex(combo->findData(QStringLiteral("vanilla-262")));
    QCoreApplication::processEvents();
    QCOMPARE(controller->snapshot().selectedId, QStringLiteral("vanilla-262"));
    QCOMPARE(selectedSpy.count(), 2);

    controller->setLaunchActive(true);
    QCoreApplication::processEvents();
    QVERIFY(!pick->isEnabled());
    QVERIFY(!combo->isEnabled());
    controller->setLaunchActive(false);
    QCoreApplication::processEvents();
    QVERIFY(pick->isEnabled());
    QVERIFY(combo->isEnabled());

    gateway.home.sessionActive = true;
    gateway.publish();
    QCoreApplication::processEvents();
    QVERIFY(!pick->isEnabled());
    QVERIFY(!combo->isEnabled());
    gateway.home.sessionActive = false;
    gateway.publish();
    QCoreApplication::processEvents();
    QVERIFY(pick->isEnabled());

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.zip")),
    });
    QCoreApplication::processEvents();
    QVERIFY(controller->snapshot().phase == application::ImportPhase::Inspecting);
    QVERIFY(!pick->isEnabled());
    QVERIFY(!combo->isEnabled());
    auto* cancel = buttonWithText(*window.homePage(), QStringLiteral("取消导入"));
    QVERIFY(cancel);
    cancel->click();
    workspace.failInspect(application::ImportError::Cancelled);
    QCoreApplication::processEvents();
    QVERIFY(controller->snapshot().phase == application::ImportPhase::Cancelled);
    QVERIFY(pick->isEnabled());
    QVERIFY(combo->isEnabled());

    auto* section = window.homePage()->findChild<QWidget*>(QStringLiteral("ModpackSection"));
    QVERIFY(section);
    QVERIFY(section->isVisible());
    controller.reset();
    QCoreApplication::processEvents();
    QVERIFY(!section->isVisible());
}

void TestUi::homeModpackControllerReplacementIgnoresOldController()
{
    FakeModpackWorkspace firstWorkspace;
    FakeModpackWorkspace secondWorkspace;
    auto first = std::make_unique<application::ModpackController>(
        firstWorkspace, bundledModpackInstance());
    auto second = std::make_unique<application::ModpackController>(
        secondWorkspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(first.get());
    window.setModpackController(second.get());
    window.show();
    QCoreApplication::processEvents();

    auto* section = window.homePage()->findChild<QWidget*>(QStringLiteral("ModpackSection"));
    QVERIFY(section);
    QVERIFY(section->isVisible());
    first.reset();
    QCoreApplication::processEvents();
    QVERIFY(section->isVisible());

    sendUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/second.zip")),
    });
    QCoreApplication::processEvents();
    QCOMPARE(firstWorkspace.inspectCalls.size(), std::size_t(0));
    QCOMPARE(secondWorkspace.inspectCalls.size(), std::size_t(1));
    QVERIFY(second->snapshot().phase == application::ImportPhase::Inspecting);
    auto* phase = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackPhase"));
    QVERIFY(phase);
    QCOMPARE(phase->text(), QStringLiteral("读取整合包"));
}

void TestUi::homeImportAcceptsNativeQtDropEvents()
{
    FakeModpackWorkspace workspace;
    application::ModpackController controller(workspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(&controller);
    window.show();
    QCoreApplication::processEvents();

    auto* page = window.homePage();
    auto* hint = page->findChild<QLabel*>(QStringLiteral("ModpackHint"));
    QVERIFY(page->isVisible());
    QVERIFY(page->acceptDrops());
    QVERIFY(hint);

    sendNativeUrlDrop(*page, {QUrl(QStringLiteral("https://example.com/pack.zip"))});
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(0));
    QVERIFY(hint->isVisible());
    QVERIFY(hint->text().contains(QStringLiteral("远程链接")));

    sendNativeUrlDrop(*page, {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/a.zip")),
        QUrl::fromLocalFile(QStringLiteral("C:/packs/b.zip")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(0));
    QVERIFY(hint->text().contains(QStringLiteral("一次只能导入一个")));

    sendNativeUrlDrop(*page, {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/notes.txt")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(0));
    QVERIFY(hint->text().contains(QStringLiteral(".zip")));

    sendNativeUrlDrop(*page, {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.zip")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(1));
    QCOMPARE(workspace.inspectCalls.at(0).archive,
             QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.zip")).toLocalFile());
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Inspecting);

    sendNativeUrlDrop(*page, {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/while-busy.zip")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(1));
    QVERIFY(hint->text().contains(QStringLiteral("正在导入或游戏运行中")));

    controller.cancel(controller.snapshot().taskId);
    workspace.failInspect(application::ImportError::Cancelled);
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Cancelled);

    sendNativeUrlDrop(*page, {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.mrpack")),
    });
    QCOMPARE(workspace.inspectCalls.size(), std::size_t(2));
    QCOMPARE(workspace.inspectCalls.at(1).archive,
             QUrl::fromLocalFile(QStringLiteral("C:/packs/campus.mrpack")).toLocalFile());
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Inspecting);
}

void TestUi::homeImportRendersPackAndErrorContextAsPlainText()
{
    FakeModpackWorkspace workspace;
    application::ModpackController controller(workspace, bundledModpackInstance());
    UiGateway gateway;
    gateway.home = realHomeSnapshot();
    hustmcl::app::MainWindow window(&gateway, hustmcl::app::MainWindow::Mode::Real);
    window.setModpackController(&controller);
    window.show();
    QCoreApplication::processEvents();

    const QString maliciousName = QStringLiteral("<b>Evil Pack</b><img src=x>");
    const QString maliciousContext = QStringLiteral("<i>context</i><script>alert(1)</script>");
    sendNativeUrlDrop(*window.homePage(), {
        QUrl::fromLocalFile(QStringLiteral("C:/packs/evil.zip")),
    });
    core::Modpack pack;
    pack.name = maliciousName.toStdString();
    pack.version = "1.0";
    pack.minecraft = "1.21.1";
    workspace.completeInspect(pack);
    QCoreApplication::processEvents();

    auto* packName = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackPackName"));
    auto* errorLabel = window.homePage()->findChild<QLabel*>(QStringLiteral("ModpackError"));
    QVERIFY(packName);
    QVERIFY(errorLabel);
    QCOMPARE(packName->textFormat(), Qt::PlainText);
    QCOMPARE(packName->text(), maliciousName);

    workspace.failPrepare(application::ImportError::InvalidManifest, maliciousContext);
    QCoreApplication::processEvents();
    QVERIFY(controller.snapshot().phase == application::ImportPhase::Failed);
    QCOMPARE(errorLabel->textFormat(), Qt::PlainText);
    QVERIFY(errorLabel->text().contains(maliciousContext));
    QVERIFY(errorLabel->text().contains(QStringLiteral("整合包清单")));
}

void TestUi::releaseArtworkMatchesBuildMode()
{
#ifdef HUSTMCL_PUBLIC_RELEASE
    QVERIFY(!QFile(QStringLiteral(":/brand/xiaoke-campus.jpg")).exists());
    QVERIFY(QFile(QStringLiteral(":/brand/public-campus.svg")).exists());
#else
    QVERIFY(QFile(QStringLiteral(":/brand/xiaoke-campus.jpg")).exists());
    QVERIFY(!QFile(QStringLiteral(":/brand/public-campus.svg")).exists());
#endif

    hustmcl::ui::HomePage home;
    auto* homeCaption = home.findChild<QLabel*>(QStringLiteral("SidebarCaption"));
    QVERIFY(homeCaption);
    hustmcl::ui::Sidebar sidebar;
    auto* sidebarCaption = sidebar.findChild<QLabel*>(QStringLiteral("SidebarCaption"));
    QVERIFY(sidebarCaption);
#ifdef HUSTMCL_PUBLIC_RELEASE
    QVERIFY(!homeCaption->text().contains(QStringLiteral("小科")));
    QVERIFY(!sidebarCaption->text().contains(QStringLiteral("小科")));
    QVERIFY(homeCaption->text().contains(QStringLiteral("校园")));
    QVERIFY(sidebarCaption->text().contains(QStringLiteral("校园")));
#else
    QVERIFY(homeCaption->text().contains(QStringLiteral("小科")));
    QVERIFY(sidebarCaption->text().contains(QStringLiteral("小科")));
#endif

    home.resize(1120, 720);
    home.show();
    QCoreApplication::processEvents();
    QPixmapCache::clear();
    const QPixmap rendered = home.grab();
    QVERIFY(!rendered.isNull());
    QPixmap cached;
#ifdef HUSTMCL_PUBLIC_RELEASE
    QVERIFY(!QPixmapCache::find(QStringLiteral("hustmcl/campus/xiaoke-campus"), &cached));
#endif
}

void TestUi::emptyModpackInstanceListIsExplained()
{
    hustmcl::ui::HomePage page;
    page.setModpackAvailable(true);
    application::ModpackSnapshot modpack;
    page.setModpackSnapshot(modpack);

    auto* combo = page.findChild<QComboBox*>(QStringLiteral("ModpackInstanceCombo"));
    QVERIFY(combo);
    QCOMPARE(combo->count(), 1);
    QCOMPARE(combo->itemText(0), QStringLiteral("暂无可用实例"));
    QVERIFY(!combo->isEnabled());

    application::HomeSnapshot home;
    home.offlineMode = true;
    home.account = {application::AccountState::Offline, QString(),
                    QStringLiteral("本地离线身份")};
    home.instance = {QString(), QString(), QString(), QString(),
                     application::InstanceState::Missing};
    home.environment = {application::JavaState::Unchecked, QString(),
                        QStringLiteral("x64"), QStringLiteral("随包运行时")};
    home.primaryAction = application::PrimaryAction::Launch;
    home.primaryEnabled = false;
    home.primaryLabel = QStringLiteral("启动游戏");
    home.primaryReason = QStringLiteral("请先导入整合包");
    home.statusTitle = QStringLiteral("还没有可用实例");
    home.launch.memoryMb = 2048;
    page.setSnapshot(home);

    const QString text = joinedLabelText(page);
    QVERIFY(text.contains(QStringLiteral("尚未安装实例")));
    QVERIFY(text.contains(QStringLiteral("请先导入整合包")));
    auto* bar = page.findChild<hustmcl::ui::PrimaryActionBar*>();
    QVERIFY(bar);
    QCOMPARE(bar->primaryButton()->text(), QStringLiteral("启动游戏"));
    QVERIFY(!bar->primaryButton()->isEnabled());
}

QTEST_MAIN(TestUi)
#include "test_ui.moc"
