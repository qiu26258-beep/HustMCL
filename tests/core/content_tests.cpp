#include <hustmcl/core/content.h>
#include <cstdlib>
#include <iostream>
using namespace hustmcl::core;
void check(bool ok) { if(!ok) std::abort(); }
int main() {
    ContentQuery q{"sodium","1.20.1","fabric",0,20};
    check(validContentQuery(q)); q.text="中文 &?"; check(validContentQuery(q));
    auto invalid=q; invalid.loader="../../forge"; check(!validContentQuery(invalid));
    invalid=q; invalid.offset=-1; check(!validContentQuery(invalid));
    invalid=q; invalid.limit=101; check(!validContentQuery(invalid));
    ContentFile file{"sodium.jar",{"https://cdn.modrinth.com/data/example.jar",std::string(40,'a'),12},true};
    check(validContentFile(file));
    for(const auto* name:{"../a.jar","a/b.jar","CON.jar","LPT1.jar","a.jar.","a.exe","a.jar:bad"}) { auto bad=file; bad.name=name; check(!validContentFile(bad)); }
    auto bad=file; bad.download.url="https://cdn.modrinth.com.evil.test/a"; check(!validContentFile(bad));
    bad=file; bad.download.sha1="bad"; check(!validContentFile(bad));
    ContentVersion v{"Version1","Project1","name","1","release",{"1.20.1"},{"fabric"},{file},{}};
    check(contentDownloadFile(v,q).has_value());
    auto mismatch=q; mismatch.loader="forge"; check(!contentDownloadFile(v,mismatch));
    mismatch=q; mismatch.gameVersion="26.2"; check(!contentDownloadFile(v,mismatch));
    v.files.push_back(file); check(!contentDownloadFile(v,q));
    v.files[0].primary=false; v.files[1].primary=false; check(!contentDownloadFile(v,q));
    v.files.pop_back(); check(contentDownloadFile(v,q).has_value());
    v.projectId="../bad"; check(!contentDownloadFile(v,q));
    std::cout<<"content rules passed\n";
}
