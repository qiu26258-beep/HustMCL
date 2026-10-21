#include "demo_controller.h"
#include "main_window.h"

#include "home_page.h"
#include "install_page.h"
#include "primary_action_bar.h"
#include "sidebar.h"

#include <QCoreApplication>
#include <QLabel>
#include <QListWidget>
#include <QPixmapCache>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>

namespace application = hustmcl::application;

class TestUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void navigationSwitchesPages();
    void primaryActionComesFromSnapshot();
    void unknownTotalHasNoPercentage();
    void knownTotalShowsProgress();
    void cancelRequiresApplicationConfirmation();
    void committingCannotBeCancelled();
    void failureAndRetryReplaceTask();
    void staleCancellationDoesNotReplaceNewTask();
    void longPathElidesAndRepeatedRenderKeepsWidgetCount();
    void settingsAndLogsAreDisabled();
};

void TestUi::initTestCase()
{
    Q_INIT_RESOURCE(resources);
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

    QStringList texts;
    for (QLabel* label : page.findChildren<QLabel*>()) {
        texts.append(label->text());
    }
    const QString joined = texts.join(QLatin1Char('\n'));
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
    QStringList texts;
    for (QLabel* label : page.findChildren<QLabel*>()) {
        texts.append(label->text());
    }
    QVERIFY(texts.join(QLatin1Char('\n')).contains(QLatin1Char('%')));
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
    QStringList texts;
    for (QLabel* label : page.findChildren<QLabel*>()) {
        texts.append(label->text());
    }
    QVERIFY(texts.join(QLatin1Char('\n')).contains(QStringLiteral("正在取消")));
    QVERIFY(!bar->secondaryButton()->isEnabled());

    controller.finishCancellation(taskId);
    QCoreApplication::processEvents();
    QVERIFY(controller.installSnapshot().task.state == application::TaskState::Cancelled);
    texts.clear();
    for (QLabel* label : page.findChildren<QLabel*>()) {
        texts.append(label->text());
    }
    QVERIFY(texts.join(QLatin1Char('\n')).contains(QStringLiteral("已取消")));
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

    const int before = page.findChildren<QWidget*>().size();
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
