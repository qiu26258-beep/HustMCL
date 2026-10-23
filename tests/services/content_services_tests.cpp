#include <hustmcl/services/modrinth_content.h>
#include <hustmcl/services/content_download_store.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdlib>
#include <iostream>
using namespace hustmcl;
using namespace application;
void check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::abort(); } }
QByteArray searchBody() {
    return R"({"offset":0,"limit":20,"total_hits":1,"hits":[{"project_id":"Project1","project_type":"mod","slug":"example","title":"Example","description":"description","downloads":12}]})";
}
QByteArray versionsBody() {
    return R"([{"id":"Version1","project_id":"Project1","name":"one","version_number":"1","version_type":"release","game_versions":["1.20.1"],"loaders":["fabric"],"files":[{"filename":"example.jar","url":"https://cdn.modrinth.com/data/example.jar","size":3,"primary":true,"hashes":{"sha1":"a9993e364706816aba3e25717850c26c9cd0d89d"}}],"dependencies":[{"project_id":"Depend01","version_id":null,"dependency_type":"required"}]}])";
}
struct FakeAcquirer:FileAcquirer {
    QString cache; Completion held; bool hold=false;
    void acquire(const QString&,const QString&,const core::DownloadDescriptor&,Completion done) override {
        if(hold) held=std::move(done); else done({FileError::None,cache,false});
    }
    void cancel(const QString&) override {}
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    check(std::holds_alternative<core::ContentPage>(services::parseModrinthSearch(searchBody())),"search parse");
    check(std::holds_alternative<ContentError>(services::parseModrinthSearch("{}")),"invalid schema");
    auto wrong=QJsonDocument::fromJson(searchBody()).object(); wrong["offset"]=-1;
    check(std::holds_alternative<ContentError>(services::parseModrinthSearch(QJsonDocument(wrong).toJson())),"invalid pagination");
    auto parsed=services::parseModrinthVersions(versionsBody(),"Project1");
    check(std::holds_alternative<std::vector<core::ContentVersion>>(parsed),"version parse");
    const auto version=std::get<std::vector<core::ContentVersion>>(parsed).front();
    check(version.dependencies.size()==1 && version.dependencies.front().type=="required","dependencies");
    check(std::holds_alternative<ContentError>(services::parseModrinthVersions(versionsBody(),"OtherPr1")),"project isolation");
    QTcpServer server; check(server.listen(QHostAddress::LocalHost),"listen");
    QByteArray body=searchBody(),request; int status=200; bool stall=false;
    QObject::connect(&server,&QTcpServer::newConnection,&app,[&] {
        while(auto* socket=server.nextPendingConnection()) {
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket] {
                auto bytes=socket->property("buffer").toByteArray()+socket->readAll();
                if(!bytes.contains("\r\n\r\n")) { socket->setProperty("buffer",bytes); return; }
                request=bytes; if(stall) return;
                socket->write("HTTP/1.1 "+QByteArray::number(status)+" Response\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);
                socket->disconnectFromHost();
            });
        }
    });
    services::ModrinthContentSource source({QUrl(QString("http://127.0.0.1:%1/v2/").arg(server.serverPort())),100,true});
    auto search=[&](bool cancel=false) {
        QEventLoop loop; bool finished=false; ContentResult<core::ContentPage> output=ContentError::Network;
        QTimer::singleShot(3000,&loop,&QEventLoop::quit);
        source.search("request",{"a&?# 中文","1.20.1","fabric",0,20},[&](auto result) { output=std::move(result); finished=true; loop.quit(); });
        if(cancel) source.cancel("request");
        if(!finished) loop.exec(); check(finished,"request settles"); return output;
    };
    check(std::holds_alternative<core::ContentPage>(search()),"HTTP search");
    check(request.contains("User-Agent: HustMCL/") && !request.contains("Authorization:"),"identified public request");
    check(request.contains("%26") && request.contains("%23") && request.contains("facets="),"query encoding");
    status=429; check(std::get<ContentError>(search())==ContentError::RateLimited,"429 mapping");
    status=302; check(std::get<ContentError>(search())==ContentError::InvalidResponse,"no redirects");
    status=200; body=QByteArray(4*1024*1024+1,'x'); check(std::get<ContentError>(search())==ContentError::InvalidResponse,"body bound");
    body=searchBody(); stall=true; check(std::get<ContentError>(search())==ContentError::Timeout,"timeout");
    check(std::get<ContentError>(search(true))==ContentError::Cancelled,"cancel");
    QTemporaryDir root; check(root.isValid(),"root");
    FakeAcquirer acquirer; acquirer.cache=QDir(root.path()).filePath("cached");
    { QFile cache(acquirer.cache); check(cache.open(QIODevice::WriteOnly) && cache.write("abc")==3,"cache fixture"); }
    services::QtContentDownloadStore store(root.path(),acquirer);
    ContentDownloadRequest download{version.projectId,version.id,version.files.front()};
    auto acquire=[&](bool cancel=false) {
        QEventLoop loop; bool finished=false; ContentResult<ContentDownloadResult> output=ContentError::Network;
        QTimer::singleShot(3000,&loop,&QEventLoop::quit);
        acquirer.hold=cancel;
        store.acquire("download",download,[&](auto result) { output=std::move(result); finished=true; loop.quit(); });
        if(cancel) { store.cancel("download"); acquirer.held({FileError::None,acquirer.cache,false}); }
        if(!finished) loop.exec(); check(finished,"publication settles"); return output;
    };
    auto output=acquire(); check(std::holds_alternative<ContentDownloadResult>(output),"named publication");
    const auto path=std::get<ContentDownloadResult>(output).path;
    check(path.endsWith("content-downloads/modrinth/Project1/Version1/example.jar"),"isolated named destination");
    check(std::get<ContentDownloadResult>(acquire()).reused,"existing verified file reused");
    { QFile target(path); check(target.open(QIODevice::WriteOnly|QIODevice::Truncate),"open target"); target.write("foreign"); }
    check(std::get<ContentError>(acquire())==ContentError::Conflict,"no overwrite");
    { QFile target(path); check(target.open(QIODevice::ReadOnly) && target.readAll()=="foreign","conflict preserved"); }
    download.versionId="Version2"; check(std::get<ContentError>(acquire(true))==ContentError::Cancelled,"cancel publication");
    check(!QFileInfo::exists(QDir(root.path()).filePath("content-downloads/modrinth/Project1/Version2/example.jar")),"cancel has no published file");
    download.file.name="../escape.jar"; check(std::get<ContentError>(acquire())==ContentError::InvalidRequest,"path validation");
    check(!QFileInfo::exists(QDir(root.path()).filePath("game")),"never writes game");
    std::cout<<"content services scenarios passed\n";
}
