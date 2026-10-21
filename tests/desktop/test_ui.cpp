#include "demo_controller.h"
#include "main_window.h"

#include "copy.h"
#include "home_page.h"
#include "install_page.h"
#include "install_stages.h"
#include "primary_action_bar.h"
#include "sidebar.h"
#include "theme.h"

#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QPixmapCache>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QStyle>
#include <QStyleFactory>
#include <QTest>

#include <cmath>
#include <limits>

namespace application = hustmcl::application;

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
}

void TestUi::navigationSwitchesPages()
{
    hustmcl::app::DemoController controller;
    hustmcl::app::MainWindow window(&controller);
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
    QVERIFY(QPixmapCache::find(QStringLiteral("hustmcl/campus/xiaoke-campus"), &cached));
}

void TestUi::noDecorationHidesArtwork()
{
    hustmcl::app::DemoController controller;
    hustmcl::app::MainWindow window(&controller);
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
    hustmcl::app::MainWindow window(&controller);
    window.show();
    QCoreApplication::processEvents();
    auto* nav = window.findChild<QListWidget*>(QStringLiteral("Navigation"));
    QVERIFY(nav);
    QCOMPARE(nav->count(), 4);
    QVERIFY(!(nav->item(2)->flags() & Qt::ItemIsEnabled));
    QVERIFY(!(nav->item(3)->flags() & Qt::ItemIsEnabled));
    auto* note = window.findChild<QLabel*>(QStringLiteral("NavNote"));
    QVERIFY(note);
    QVERIFY(note->text().contains(QStringLiteral("后续批次")));
}

QTEST_MAIN(TestUi)
#include "test_ui.moc"
