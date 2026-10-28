#include <hustmcl/application/modpack_import.h>
#include <QCoreApplication>
#include <iostream>
using namespace hustmcl;
using namespace application;
struct Workspace:ModpackWorkspace {
    std::function<void(ImportResult<core::Modpack>)> inspected;
    Done prepared,installed;
    std::function<void(ImportResult<InstalledInstance>)> committed;
    int prepares=0,files=0,commits=0,cancels=0,releases=0;
    void inspect(const QString&,const QString&,decltype(inspected) f)override{inspected=std::move(f);}
    void inspectVanilla(const QString&,const QString&,decltype(inspected) f)override{inspected=std::move(f);}
    void prepareGame(const QString&,Progress,Done f)override{++prepares;prepared=std::move(f);}
    void installFiles(const QString&,Progress,Done f)override{++files;installed=std::move(f);}
    void commit(const QString&,decltype(committed) f)override{++commits;committed=std::move(f);}
    void cancel(const QString&)override{++cancels;}
    void release(const QString&)override{++releases;}
    void listInstalled(std::function<void(ImportResult<std::vector<InstalledInstance>>)> f)override{f(std::vector<InstalledInstance>{});}
};
int main(int argc,char**argv){
 QCoreApplication app(argc,argv); Workspace w;
 { Workspace emptyWorkspace; ModpackController empty(emptyWorkspace,{});
   if(!empty.snapshot().instances.empty()||empty.selectInstance(""))return 20;
   if(!empty.importFile("D:/first.mrpack"))return 21;
 }
 ModpackController c(w,{"bundled","Vanilla","26.2","D:/test","26.2","D:/java.exe"});
 int selected=0;QObject::connect(&c,&ModpackController::instanceSelected,[&](auto){++selected;});
 const core::Modpack pack{"Test","1","1.21.1","",core::PackLoader::Vanilla,{}};
 if(!c.importFile("D:/pack.zip")||c.importFile("D:/second.zip"))return 1;
 auto first=c.snapshot().taskId;auto stale=w.inspected;
 if(!c.cancel(first)||c.snapshot().phase!=ImportPhase::Cancelling)return 2;
 w.inspected(pack);if(c.snapshot().phase!=ImportPhase::Cancelled||w.prepares)return 3;
 if(!c.retry(first))return 4;
 stale(pack);if(w.prepares)return 5;
 w.inspected(pack);if(w.prepares!=1)return 6;
 w.prepared(ImportFailure{ImportError::Network,{}});
 if(c.snapshot().phase!=ImportPhase::Failed||w.files||w.commits)return 7;
 if(!c.retry(c.snapshot().taskId))return 8;
 w.inspected(pack);w.prepared(std::monostate{});w.installed(std::monostate{});
 if(c.snapshot().phase!=ImportPhase::Committing||c.cancel(c.snapshot().taskId)||selected)return 9;
 w.committed(InstalledInstance{"pack","Pack","1.21.1","D:/pack","1.21.1","D:/java.exe"});
 if(c.snapshot().phase!=ImportPhase::Succeeded||selected!=1||c.snapshot().instances.size()!=2)return 10;
 c.setLaunchActive(true);if(c.selectInstance("bundled")||c.importFile("D:/pack.zip"))return 11;
 c.setLaunchActive(false);if(!c.selectInstance("bundled")||selected!=2)return 12;
 // Cancelling waits for the active phase to settle and cannot publish a partial installation.
 c.importFile("D:/pack.zip");w.inspected(pack);c.cancel(c.snapshot().taskId);w.prepared(std::monostate{});
 if(c.snapshot().phase!=ImportPhase::Cancelled||w.commits!=1)return 13;
 { Workspace vanilla; ModpackController v(vanilla,{});
   if(v.installVanilla("../bad")||!v.installVanilla("1.21.1")||v.installVanilla("26.2"))return 30;
   if(v.snapshot().vanillaVersion!="1.21.1"||!v.snapshot().archivePath.isEmpty())return 31;
   vanilla.inspected(pack);vanilla.prepared(ImportFailure{ImportError::Network,{}});
   if(!v.retry(v.snapshot().taskId)||v.snapshot().vanillaVersion!="1.21.1")return 32;
   vanilla.inspected(pack);vanilla.prepared(std::monostate{});vanilla.installed(std::monostate{});
   vanilla.committed(InstalledInstance{"vanilla","Vanilla","1.21.1","D:/v","1.21.1","D:/java.exe"});
   if(v.snapshot().phase!=ImportPhase::Succeeded)return 33;
   v.setLaunchActive(true);if(v.installVanilla("26.2"))return 34;
 }
 std::cout<<"phase ordering, failure/cancel, stale callbacks, commit and selection passed\n";
}
