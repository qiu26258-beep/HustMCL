#include <hustmcl/services/modpack_workspace.h>
#include <hustmcl/services/local_game.h>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <iostream>
using namespace hustmcl;
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);if(argc!=4){std::cerr<<"Usage: hustmcl_modpack_probe DATA_ROOT BUNDLED_JAVA PACK_OR_--vanilla=VERSION\n";return 2;}
 const auto root=QDir::fromNativeSeparators(QString::fromLocal8Bit(argv[1]));const auto java=QDir::fromNativeSeparators(QString::fromLocal8Bit(argv[2]));
 services::QtModpackWorkspace workspace(root,java,{});application::ModpackController controller(workspace,{"bundled","Vanilla","26.2",root,"26.2",java});
 QObject::connect(&controller,&application::ModpackController::changed,&app,[&](const auto& s){
  static int phase=-1;static std::uint64_t files=0;
  if(phase!=static_cast<int>(s.phase)||s.progress.completedFiles>=files+200){
   phase=static_cast<int>(s.phase);files=s.progress.completedFiles;std::cerr<<"phase="<<phase<<" files="<<files<<"/"<<s.progress.totalFiles<<std::endl;
  }
  if(s.phase==application::ImportPhase::Succeeded){const auto i=s.instances.back();
   const bool verified=std::holds_alternative<services::LocalGame>(services::verifyLocalGame(i.root,i.versionId))&&services::verifyImportedPackFiles(i.root,true);
   const auto report=QJsonDocument(QJsonObject{{"status",verified?"passed":"failed"},{"instanceId",i.id},{"root",i.root},{"java",i.javaPath},{"version",i.versionId},{"gameStarted",false}}).toJson(QJsonDocument::Compact);
   std::cout<<report.constData()<<std::endl;app.exit(verified?0:4);
  }else if(s.phase==application::ImportPhase::Failed){
   const auto e=*s.error;std::cout<<QJsonDocument(QJsonObject{{"status","failed"},{"error",static_cast<int>(e.code)},{"context",e.context},{"gameStarted",false}}).toJson(QJsonDocument::Compact).constData()<<std::endl;app.exit(3);
  }
 });
 QTimer::singleShot(0,&app,[&]{const auto input=QString::fromLocal8Bit(argv[3]);
  const bool started=input.startsWith("--vanilla=")?controller.installVanilla(input.mid(10)):controller.importFile(QDir::fromNativeSeparators(input));
  if(!started){std::cerr<<"Invalid or busy installation request\n";app.exit(2);}});
 return app.exec();
}
