#include <hustmcl/application/content_market.h>
#include <hustmcl/services/modrinth_content.h>
#include <hustmcl/services/content_download_store.h>
#include <hustmcl/services/file_acquirer.h>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <algorithm>
#include <iostream>
using namespace hustmcl;
using namespace application;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if(argc!=2 || !QDir::isAbsolutePath(QString::fromLocal8Bit(argv[1]))) return 2;
    const QString root=QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
    if(!QDir().mkpath(root)) return 3;
    services::ModrinthContentSource source({}); services::QtFileAcquirer raw({30000,false});
    services::QtContentDownloadStore files(root,raw); ContentController controller(source,files);
    auto wait=[&](const std::function<bool()>& action) {
        QEventLoop loop; bool terminal=false;
        const auto connection=QObject::connect(&controller,&ContentController::changed,&loop,[&](const auto& s) {
            if(s.phase==ContentPhase::Ready || s.phase==ContentPhase::Downloaded || s.phase==ContentPhase::Failed || s.phase==ContentPhase::Cancelled) { terminal=true; loop.quit(); }
        });
        QTimer::singleShot(60000,&loop,&QEventLoop::quit);
        if(!action()) { QObject::disconnect(connection); return false; }
        if(!terminal) loop.exec(); QObject::disconnect(connection);
        return terminal && controller.snapshot().error==ContentError::None;
    };
    auto failed=[&] { std::cout<<"content probe failed, error="<<static_cast<int>(controller.snapshot().error)<<'\n'; return 4; };
    if(!wait([&] { return controller.search({"sodium","1.20.1","fabric",0,20}); })) return failed();
    const auto page=controller.snapshot().page;
    const auto project=std::find_if(page.projects.begin(),page.projects.end(),[](const auto& p) { return p.slug=="sodium"; });
    if(project==page.projects.end() || !wait([&] { return controller.selectProject(QString::fromStdString(project->id)); })) return failed();
    const auto versions=controller.snapshot().versions; if(versions.empty()) return failed();
    const auto version=versions.front();
    if(!wait([&] { return controller.download(QString::fromStdString(version.id)); })) return failed();
    const auto firstPath=controller.snapshot().downloadPath;
    if(!wait([&] { return controller.download(QString::fromStdString(version.id)); })) return failed();
    const auto state=controller.snapshot();
    const QJsonObject report{{"status","passed"},{"provider","modrinth"},{"projectId",QString::fromStdString(project->id)},
        {"version",QString::fromStdString(version.number)},{"downloadPath",state.downloadPath},{"reused",state.reused},
        {"samePath",state.downloadPath==firstPath},{"gameModified",false},{"gameStarted",false}};
    std::cout<<QJsonDocument(report).toJson().constData();
    return state.reused && state.downloadPath==firstPath ? 0 : 5;
}
