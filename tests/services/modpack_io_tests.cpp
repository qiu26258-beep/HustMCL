#include "../../src/services/modpack_io.h"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <iostream>
using namespace hustmcl;
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);QTcpServer server;if(!server.listen(QHostAddress::LocalHost,0))return 2;
 QObject::connect(&server,&QTcpServer::newConnection,[&]{
  while(auto* s=server.nextPendingConnection()){
   QObject::connect(s,&QTcpSocket::disconnected,s,&QObject::deleteLater);
   QObject::connect(s,&QTcpSocket::readyRead,s,[s]{
    const auto request=s->readAll();if(!request.contains("\r\n\r\n"))return;
    if(request.contains(" /slow "))return;
    QByteArray response;
    if(request.contains(" /redirect "))response="HTTP/1.1 302 Found\r\nLocation: /ok\r\nContent-Length: 0\r\n\r\n";
    else if(request.contains(" /blocked "))response="HTTP/1.1 302 Found\r\nLocation: http://evil.invalid/x\r\nContent-Length: 0\r\n\r\n";
    else response="HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: close\r\n\r\n"+(request.startsWith("HEAD")?QByteArray{}:QByteArray("body"));
    s->write(response);s->disconnectFromHost();
   });
  }
 });
 std::atomic_bool cancel=false;services::pack::Network net(cancel,true,150);
 const auto base=QString("http://127.0.0.1:%1").arg(server.serverPort());
 if(net.bytes(base+"/ok")!="body"||net.bytes(base+"/redirect")!="body"||!net.resolve(base+"/redirect").endsWith("/ok"))return 3;
 auto rejects=[&](const QString& url,quint64 max,application::ImportError expected){
  try{net.bytes(url,max);return false;}catch(const application::ImportFailure& f){return f.code==expected;}
 };
 if(!rejects(base+"/ok",3,application::ImportError::Integrity)
  ||!rejects(base+"/blocked",100,application::ImportError::InvalidManifest)
  ||!rejects(base+"/slow",100,application::ImportError::Network))return 4;
 QTimer::singleShot(20,&app,[&]{cancel=true;});
 if(!rejects(base+"/slow",100,application::ImportError::Cancelled))return 5;
 std::cout<<"bounded HTTP, redirects, host policy, timeout and cancellation passed\n";
}
