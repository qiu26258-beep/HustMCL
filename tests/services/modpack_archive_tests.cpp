#include "../../src/services/modpack_archive.h"
#include <QCoreApplication>
#include <QDir>
#include <iostream>
using namespace hustmcl;
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);if(argc!=2)return 2;QDir dir(QString::fromLocal8Bit(argv[1]));std::atomic_bool cancel=false;
 for(const auto& name:dir.entryList({"*.zip"},QDir::Files,QDir::Name)){
   bool accepted=false;
   try{auto p=services::pack::inspectArchive(dir.filePath(name),dir.filePath("inner.mrpack"),cancel);
       accepted=p.pack.name=="测试 Pack"&&p.pack.minecraft=="1.21.1";
       if(name=="allow-overrides.zip"){
          if(p.overrides.size()!=1||p.overrides[0].layer!=2)return 4;
          auto b=services::pack::archiveBytes(p.archive,p.overrides[0].index,1024);if(b!="client")return 5;
       }
       if(name=="allow-server-only.zip"&&!p.pack.files.empty())return 6;
   }catch(const application::ImportFailure&){}
   if(accepted!=name.startsWith("allow-")){std::cerr<<name.toStdString()<<" unexpected acceptance="<<accepted;return 1;}
 }
 cancel=true;try{services::pack::inspectArchive(dir.filePath("allow-direct.zip"),dir.filePath("inner.mrpack"),cancel);return 7;}
 catch(const application::ImportFailure& e){if(e.code!=application::ImportError::Cancelled)return 8;}
 std::cout<<"ZIP fixtures and cancellation passed\n";
}
