#include <hustmcl/application/content_market.h>
#include <QCoreApplication>
#include <cstdlib>
#include <iostream>
using namespace hustmcl;
using namespace application;
void check(bool ok) { if(!ok) std::abort(); }
struct Source:ContentSource {
    std::function<void(ContentResult<core::ContentPage>)> searchDone;
    std::function<void(ContentResult<std::vector<core::ContentVersion>>)> versionsDone;
    int searches=0,lists=0,cancels=0;
    void search(const QString&,const core::ContentQuery&,decltype(searchDone) done) override { ++searches; searchDone=std::move(done); }
    void versions(const QString&,const std::string&,const core::ContentQuery&,decltype(versionsDone) done) override { ++lists; versionsDone=std::move(done); }
    void cancel(const QString&) override { ++cancels; }
};
struct Files:ContentDownloadStore {
    std::function<void(ContentResult<ContentDownloadResult>)> done;
    int calls=0,cancels=0;
    void acquire(const QString&,const ContentDownloadRequest&,decltype(done) callback) override { ++calls; done=std::move(callback); }
    void cancel(const QString&) override { ++cancels; }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    Source source; Files files; ContentController controller(source,files);
    core::ContentQuery q{"sodium","1.20.1","fabric",0,20};
    core::ContentProject p{core::ContentProvider::Modrinth,"Project1","sodium","Sodium","description",1};
    core::ContentPage page{{p},0,20,1};
    core::ContentFile f{"sodium.jar",{"https://cdn.modrinth.com/data/a.jar",std::string(40,'a'),12},true};
    core::ContentVersion v{"Version1",p.id,"one","1","release",{"1.20.1"},{"fabric"},{f},{}};
    check(controller.search(q)); auto stale=source.searchDone;
    check(controller.search(q)); stale(page); check(controller.snapshot().phase==ContentPhase::Searching);
    source.searchDone(page); check(controller.snapshot().page.projects.size()==1);
    check(!controller.selectProject("unknown")); check(controller.selectProject("Project1"));
    auto bad=v; bad.loaders={"forge"}; source.versionsDone(std::vector<core::ContentVersion>{bad});
    check(controller.snapshot().versions.empty()); check(!controller.download("Version1") && files.calls==0);
    check(controller.selectProject("Project1")); source.versionsDone(std::vector<core::ContentVersion>{v});
    check(controller.download("Version1")); check(files.calls==1);
    check(!controller.search(q) && !controller.selectProject("Project1") && !controller.download("Version1"));
    check(!controller.cancel("stale")); check(controller.cancel(controller.snapshot().taskId));
    check(controller.snapshot().phase==ContentPhase::Cancelling && files.cancels==1);
    files.done(ContentError::Cancelled); check(controller.snapshot().phase==ContentPhase::Cancelled);
    check(controller.download("Version1")); files.done(ContentDownloadResult{"download.jar",false});
    check(controller.snapshot().phase==ContentPhase::Downloaded && controller.snapshot().downloadPath=="download.jar");
    auto oldDownload=files.done; oldDownload(ContentError::Integrity); check(controller.snapshot().phase==ContentPhase::Downloaded);
    check(controller.search(q)); check(controller.cancel(controller.snapshot().taskId)); source.searchDone(page);
    check(controller.snapshot().phase==ContentPhase::Cancelled);
    auto invalid=q; invalid.loader="bad"; check(!controller.search(invalid)); check(controller.snapshot().error==ContentError::InvalidRequest);
    {
        ContentController temporary(source,files); check(temporary.search(q));
    }
    source.searchDone(page); // Destroyed controller cannot receive stale completion.
    std::cout<<"content controller scenarios passed\n";
}
