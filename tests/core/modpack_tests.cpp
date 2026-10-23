#include <hustmcl/core/modpack.h>
#include <iostream>
using namespace hustmcl::core;
int main() {
    for(const auto* p:{"../escape","C:/x","/x","a\\b","a//b","a/./b","a/../b","a./x","a /b","nul.jar","COM1/x","file:stream","a/"})
        if(validPackPath(p)){std::cerr<<"accepted unsafe path "<<p;return 1;}
    if(!validPackPath("config/中文 file.json")||!validPackPath("mods/a+1.jar"))return 2;
    if(!packPathsConflict({"mods/A.jar","mods/a.jar"})||!packPathsConflict({"mods","mods-other","mods/a.jar"})
       ||packPathsConflict({"mods/a.jar","mods/b.jar"}))return 3;
    Modpack p{"test","1","1.21.1","0.16.14",PackLoader::Fabric,{}};
    if(!validModpack(p))return 4;
    p.files.push_back({"mods/a.jar",std::string(40,'a'),std::string(128,'b'),10,{"https://cdn.modrinth.com/a"},false});
    if(!validModpack(p))return 5;
    p.files.push_back(p.files[0]); if(validModpack(p))return 6;
    p.files.pop_back();p.files[0].size=1024ULL*1024*1024+1;if(validModpack(p))return 7;
    std::cout<<"modpack path, collision, identity and bounds passed\n";
}
