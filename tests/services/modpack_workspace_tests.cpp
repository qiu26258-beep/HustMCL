#include <hustmcl/services/modpack_workspace.h>
#include <hustmcl/services/local_game.h>
#include <hustmcl/services/version_metadata.h>
#include "../../src/services/modpack_io.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <iostream>
using namespace hustmcl;
using namespace application;
QByteArray bytes(const QString& p){QFile f(p);if(!f.open(QIODevice::ReadOnly))throw 1;return f.readAll();}
void write(const QString& p,const QByteArray& b){services::pack::write(p,b);}
QByteArray sha(const QByteArray& b){return QCryptographicHash::hash(b,QCryptographicHash::Sha1).toHex();}
bool wait(const std::function<bool()>& predicate){QElapsedTimer t;t.start();while(!predicate()&&t.elapsed()<12000){QCoreApplication::processEvents(QEventLoop::AllEvents,10);QThread::msleep(1);}return predicate();}
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);if(argc!=3)return 2;
 try{
 QTemporaryDir temp;const auto root=QDir(temp.path()).filePath("中文 安装数据");services::pack::mkdir(root);const QDir data(root),fixtures(QString::fromLocal8Bit(argv[1]));
 write(data.filePath("game/saves/keep.txt"),"old-world");
 const QByteArray engine="engine bytes";const auto digest=QString::fromLatin1(sha(engine));write(data.filePath("cache/sha1/"+digest),engine);
 const auto index=QJsonDocument(QJsonObject{{"objects",QJsonObject{{"test/sound.ogg",QJsonObject{{"hash",digest},{"size",engine.size()}}}}}}).toJson(QJsonDocument::Compact);
 const auto indexHash=QString::fromLatin1(sha(index));write(data.filePath("cache/sha1/"+indexHash),index);
 for(const auto& name:fixtures.entryList(QDir::Files))if(name.size()==40)write(data.filePath("cache/sha1/"+name),bytes(fixtures.filePath(name)));
 auto version=QJsonDocument::fromJson(bytes(QStringLiteral(HUSTMCL_FIXTURE_DIR)+"/1.21.1.json")).object();
 auto descriptor=[&](QString url){return QJsonObject{{"url",url},{"sha1",digest},{"size",engine.size()}};};
 version.insert("downloads",QJsonObject{{"client",descriptor("https://piston-data.mojang.com/test/client.jar")}});
 version.insert("assetIndex",QJsonObject{{"id","test"},{"url","https://piston-meta.mojang.com/test/assets.json"},{"sha1",indexHash},{"size",index.size()},{"totalSize",engine.size()}});
 auto logging=version.value("logging").toObject();auto client=logging.value("client").toObject();auto log=descriptor("https://piston-data.mojang.com/test/log.xml");log.insert("id","test.xml");client.insert("file",log);logging.insert("client",client);version.insert("logging",logging);
 QJsonArray libraries;for(const auto& item:version.value("libraries").toArray()){
   auto lib=item.toObject();auto a=lib.value("downloads").toObject().value("artifact").toObject();a.insert("sha1",digest);a.insert("size",engine.size());lib.insert("downloads",QJsonObject{{"artifact",a}});libraries.append(lib);
 }version.insert("libraries",libraries);const auto versionBytes=QJsonDocument(version).toJson(QJsonDocument::Compact);
 QTcpServer server;if(!server.listen(QHostAddress::LocalHost,0))return 3;
 const auto base=QString("http://127.0.0.1:%1").arg(server.serverPort());
 const auto manifest=QJsonDocument(QJsonObject{{"versions",QJsonArray{QJsonObject{{"id","1.21.1"},{"url",base+"/version"},{"sha1",QString::fromLatin1(sha(versionBytes))}}}}}).toJson();
 QObject::connect(&server,&QTcpServer::newConnection,[&]{while(auto* socket=server.nextPendingConnection()){
   QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
   QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{const auto request=socket->readAll();if(!request.contains("\r\n\r\n"))return;
     const auto body=request.contains(" /manifest ")?manifest:versionBytes;socket->write("HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();});
 }});
 services::QtModpackWorkspace::Options options;options.allowLoopbackHttp=true;options.versionManifestUrl=base+"/manifest";
 const auto java=QString::fromLocal8Bit(argv[2]);services::QtModpackWorkspace workspace(root,java,options);
 ModpackController controller(workspace,{"bundled","Vanilla","26.2",root,"26.2",java});
 if(!controller.importFile(fixtures.filePath("pack.zip")))return 4;
 if(!wait([&]{return !importActive(controller.snapshot().phase);}))return 5;
 auto state=controller.snapshot();if(state.phase!=ImportPhase::Succeeded){std::cerr<<"import failed "<<(state.error?static_cast<int>(state.error->code):-1)<<" "<<(state.error?state.error->context.toStdString():"");return 6;}
 const auto installed=state.instances.back();
 const auto checked=services::verifyLocalGame(installed.root,"1.21.1");
 if(!std::holds_alternative<services::LocalGame>(checked)){std::cerr<<"published game check "<<static_cast<int>(std::get<services::LocalGameError>(checked));return 7;}
 if(!services::verifyImportedPackFiles(installed.root)){std::cerr<<"published pack check";return 7;}
 if(bytes(QDir(installed.root).filePath("game/config/common.txt"))!="client"||bytes(QDir(installed.root).filePath("game/config/download.txt"))!="override"||bytes(data.filePath("game/saves/keep.txt"))!="old-world")return 8;
 write(QDir(installed.root).filePath("game/config/common.txt"),"user-config");if(!services::verifyImportedPackFiles(installed.root))return 9;
 const auto mod=QDir(installed.root).filePath("game/mods/sample.jar");const auto original=bytes(mod);write(mod,"corrupt");if(services::verifyImportedPackFiles(installed.root))return 10;write(mod,original);
 std::optional<ImportResult<std::vector<InstalledInstance>>> listed;
 workspace.listInstalled([&](auto r){listed=std::move(r);});if(!wait([&]{return listed.has_value();})||!std::holds_alternative<std::vector<InstalledInstance>>(*listed)||std::get<std::vector<InstalledInstance>>(*listed).size()!=1)return 11;
 controller.importFile(fixtures.filePath("pack.zip"));if(!wait([&]{return !importActive(controller.snapshot().phase);})||controller.snapshot().phase!=ImportPhase::Failed||controller.snapshot().error->code!=ImportError::Conflict)return 12;
 controller.importFile(fixtures.filePath("forge.zip"));if(!wait([&]{return !importActive(controller.snapshot().phase);})||controller.snapshot().error->code!=ImportError::UnsupportedLoader)return 13;
 controller.importFile(fixtures.filePath("failed.zip"));controller.cancel(controller.snapshot().taskId);
 if(!wait([&]{return !importActive(controller.snapshot().phase);})||controller.snapshot().phase!=ImportPhase::Cancelled)return 14;
 write(data.filePath("cache/sha1/"+digest),"bad-cache");controller.importFile(fixtures.filePath("failed.zip"));
 if(!wait([&]{return !importActive(controller.snapshot().phase);})||controller.snapshot().phase!=ImportPhase::Failed||controller.snapshot().error->code!=ImportError::Integrity)return 15;
 if(QDir(data.filePath("instances")).entryList(QDir::Dirs|QDir::NoDotAndDotDot).size()!=1||bytes(data.filePath("game/saves/keep.txt"))!="old-world")return 16;
 std::cout<<"staged installation, publish, reread, overrides, mod verification, cancellation, conflict, unsupported loader and corrupt cache passed\n";
 }catch(const ImportFailure& e){std::cerr<<"unexpected "<<static_cast<int>(e.code)<<" "<<e.context.toStdString();return 20;}catch(...){return 21;}
}
