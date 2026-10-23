#include "modpack_archive.h"
#include "miniz.h"
#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSaveFile>
#include <QUrl>
#include <cmath>

namespace hustmcl::services::pack {
namespace {
using application::ImportError;
[[noreturn]] void fail(ImportError e, QString context={}) {throw application::ImportFailure{e,std::move(context)};}
class Zip {
public:
    QFile file; mz_zip_archive zip{};
    explicit Zip(const QString& path):file(path) {
        if(!file.open(QIODevice::ReadOnly)||file.size()<22||file.size()>8LL*1024*1024*1024)fail(ImportError::InvalidArchive);
        zip.m_pIO_opaque=&file;
        zip.m_pRead=[](void* p,mz_uint64 offset,void* bytes,size_t size)->size_t {
            auto& f=*static_cast<QFile*>(p);
            if(!f.seek(static_cast<qint64>(offset)))return 0;
            const auto read=f.read(static_cast<char*>(bytes),static_cast<qint64>(size));
            return read<0?0:static_cast<size_t>(read);
        };
        if(!mz_zip_reader_init(&zip,static_cast<mz_uint64>(file.size()),0))fail(ImportError::InvalidArchive);
    }
    ~Zip(){mz_zip_reader_end(&zip);}
    Zip(const Zip&)=delete;
    mz_zip_archive_file_stat stat(unsigned i) {
        mz_zip_archive_file_stat s{};if(!mz_zip_reader_file_stat(&zip,i,&s))fail(ImportError::InvalidArchive);return s;
    }
};
QString string(const QJsonValue& v,int maximum=512) {
    if(!v.isString()||v.toString().isEmpty()||v.toString().size()>maximum)fail(ImportError::InvalidManifest);
    const auto s=v.toString();
    for(qsizetype i=0;i<s.size();++i){
        const auto c=s[i];if(c.unicode()<32||c.unicode()==127)fail(ImportError::InvalidManifest);
        if(c.isHighSurrogate()){if(i+1>=s.size()||!s[++i].isLowSurrogate())fail(ImportError::InvalidManifest);}
        else if(c.isLowSurrogate())fail(ImportError::InvalidManifest);
    }
    return s;
}
quint64 integer(const QJsonValue& v) {
    const auto d=v.toDouble(-1);if(!v.isDouble()||d<0||d>16.0*1024*1024*1024||std::floor(d)!=d)fail(ImportError::InvalidManifest);
    return static_cast<quint64>(d);
}
bool allowedContentUrl(const QString& value) {
    const QUrl u(value,QUrl::StrictMode);
    static const QStringList hosts{"cdn.modrinth.com","github.com","raw.githubusercontent.com","gitlab.com","edge.forgecdn.net","mediafilez.forgecdn.net","media.forgecdn.net"};
    return u.isValid()&&u.scheme()=="https"&&u.port(-1)==-1&&u.userInfo().isEmpty()&&!u.hasFragment()
        &&!value.contains('\\')&&!value.contains(' ')&&hosts.contains(u.host());
}
core::Modpack parseManifest(const QByteArray& bytes) {
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError||!doc.isObject())fail(ImportError::InvalidManifest);
    const auto json=doc.object();
    if(json.value("formatVersion")!=1||json.value("game")!="minecraft"||!json.value("dependencies").isObject()
        ||!json.value("files").isArray())fail(ImportError::InvalidManifest);
    core::Modpack result;result.name=string(json.value("name")).toStdString();result.version=string(json.value("versionId"),256).toStdString();
    const auto dependencies=json.value("dependencies").toObject();
    result.minecraft=string(dependencies.value("minecraft"),80).toStdString();
    bool loader=false;
    for(auto it=dependencies.begin();it!=dependencies.end();++it){
        if(it.key()=="minecraft")continue;
        if(loader)fail(ImportError::UnsupportedLoader,"multiple-loaders");
        loader=true;
        if(it.key()=="fabric-loader")result.loader=core::PackLoader::Fabric;
        else if(it.key()=="forge")result.loader=core::PackLoader::Forge;
        else if(it.key()=="neoforge"||it.key()=="neo-forge")result.loader=core::PackLoader::NeoForge;
        else if(it.key()=="quilt-loader")result.loader=core::PackLoader::Quilt;
        else fail(ImportError::UnsupportedLoader,it.key());
        result.loaderVersion=string(it.value(),80).toStdString();
    }
    const auto files=json.value("files").toArray();if(files.size()>50000)fail(ImportError::InvalidManifest);
    for(const auto& item:files){
        if(!item.isObject())fail(ImportError::InvalidManifest);
        const auto f=item.toObject();core::PackFile entry;entry.path=string(f.value("path"),220).toStdString();
        if(!core::validPackPath(entry.path))fail(ImportError::UnsafePath);
        if(f.contains("env")){
            if(!f.value("env").isObject())fail(ImportError::InvalidManifest);
            const auto env=f.value("env").toObject();const auto client=string(env.value("client"),16);
            if(client=="unsupported")continue;
            if(client!="required"&&client!="optional")fail(ImportError::InvalidManifest);
            entry.optional=client=="optional";
        }
        if(!f.value("hashes").isObject()||!f.value("downloads").isArray())fail(ImportError::InvalidManifest);
        const auto hashes=f.value("hashes").toObject();
        entry.sha1=string(hashes.value("sha1"),40).toLower().toStdString();
        entry.sha512=string(hashes.value("sha512"),128).toLower().toStdString();entry.size=integer(f.value("fileSize"));
        for(const auto& url:f.value("downloads").toArray()){
            const auto address=string(url,4096);if(!allowedContentUrl(address))fail(ImportError::InvalidManifest,"download-host");
            entry.urls.push_back(address.toStdString());
        }
        result.files.push_back(std::move(entry));
    }
    if(!core::validModpack(result))fail(ImportError::InvalidManifest);
    std::vector<std::string> folded;
    for(const auto& f:result.files)folded.push_back(QString::fromStdString(f.path).toCaseFolded().toStdString());
    if(core::packPathsConflict(folded))fail(ImportError::UnsafePath);
    return result;
}
}
std::vector<ArchiveEntry> archiveEntries(const QString& path) {
    Zip z(path);const auto count=mz_zip_reader_get_num_files(&z.zip);if(count>50000)fail(ImportError::InvalidArchive,"entry-limit");
    std::vector<ArchiveEntry> entries;quint64 total=0;QMap<QString,bool> seen;
    for(unsigned i=0;i<count;++i){
        const auto s=z.stat(i);const auto length=mz_zip_reader_get_filename(&z.zip,i,nullptr,0);
        if(length<2||length>1024||s.m_is_encrypted||(s.m_method!=0&&s.m_method!=8))fail(ImportError::InvalidArchive);
        QByteArray raw(static_cast<qsizetype>(length),0);mz_zip_reader_get_filename(&z.zip,i,raw.data(),length);raw.chop(1);
        QString name=QString::fromUtf8(raw);
        if(name.toUtf8()!=raw||name.contains(QChar(0)))fail(ImportError::InvalidArchive,"filename-encoding");
        const bool directory=s.m_is_directory;
        if(directory&&name.endsWith('/'))name.chop(1);
        if(!core::validPackPath(name.toStdString()))fail(ImportError::UnsafePath);
        const auto type=(s.m_external_attr>>16)&0170000;
        if((type!=0&&type!=0100000&&type!=0040000)||(s.m_external_attr&0x400))fail(ImportError::UnsafePath,"archive-link");
        total+=s.m_uncomp_size;
        if(s.m_uncomp_size>8ULL*1024*1024*1024||total>16ULL*1024*1024*1024)fail(ImportError::InvalidArchive,"size-limit");
        const auto key=name.toCaseFolded();if(seen.contains(key))fail(ImportError::UnsafePath,"duplicate-entry");
        seen.insert(key,directory);entries.push_back({i,name,s.m_uncomp_size,directory});
    }
    for(auto it=seen.begin();it!=seen.end();++it){
        if(it.value())continue;
        auto next=seen.lowerBound(it.key()+'/');
        if(next!=seen.end()&&next.key().startsWith(it.key()+'/'))fail(ImportError::UnsafePath,"file-directory-conflict");
    }
    return entries;
}
QByteArray archiveBytes(const QString& path,unsigned index,quint64 maximum){
    Zip z(path);const auto s=z.stat(index);if(s.m_is_directory||s.m_uncomp_size>maximum)fail(ImportError::InvalidArchive,"entry-size");
    QByteArray bytes(static_cast<qsizetype>(s.m_uncomp_size),0);
    if(!mz_zip_reader_extract_to_mem(&z.zip,index,bytes.data(),static_cast<size_t>(bytes.size()),0))fail(ImportError::InvalidArchive,"crc");
    return bytes;
}
void extractEntry(const QString& path,unsigned index,const QString& target,const std::atomic_bool& cancel){
    Zip z(path);const auto s=z.stat(index);if(s.m_is_directory)fail(ImportError::InvalidArchive);
    QSaveFile out(target);if(!out.open(QIODevice::WriteOnly))fail(ImportError::Storage);
    struct Output {QSaveFile& file;const std::atomic_bool& cancel;} output{out,cancel};
    const auto writer=[](void* ptr,mz_uint64 offset,const void* bytes,size_t size)->size_t {
        auto& o=*static_cast<Output*>(ptr);if(o.cancel||o.file.pos()!=static_cast<qint64>(offset))return 0;
        const auto wrote=o.file.write(static_cast<const char*>(bytes),static_cast<qint64>(size));return wrote<0?0:static_cast<size_t>(wrote);
    };
    if(!mz_zip_reader_extract_to_callback(&z.zip,index,writer,&output,0)){
        if(cancel)fail(ImportError::Cancelled);
        fail(ImportError::InvalidArchive,"crc");
    }
    if(cancel)fail(ImportError::Cancelled);
    if(static_cast<quint64>(out.size())!=s.m_uncomp_size||!out.commit())fail(ImportError::Storage);
}
ArchivePack inspectArchive(const QString& archive,const QString& innerFile,const std::atomic_bool& cancel){
    auto entries=archiveEntries(archive);QString selectedArchive=archive;
    std::vector<ArchiveEntry> manifests,inners;
    auto find=[&]{
        manifests.clear();inners.clear();
        for(const auto& e:entries){if(e.directory||e.path.count('/')>1)continue;
            const auto name=e.path.section('/',-1);
            if(name=="modrinth.index.json")manifests.push_back(e);
            if(name=="modpack.mrpack"||name=="modpack.zip")inners.push_back(e);
        }
    };find();
    if(manifests.empty()&&inners.size()==1){
        extractEntry(archive,inners[0].index,innerFile,cancel);selectedArchive=innerFile;entries=archiveEntries(innerFile);find();
        if(!inners.empty())fail(ImportError::InvalidArchive,"nested-limit");
    }else if(!inners.empty())fail(ImportError::InvalidArchive,"ambiguous-pack");
    if(manifests.size()!=1)fail(manifests.empty()?ImportError::UnsupportedFormat:ImportError::InvalidArchive);
    const auto entry=manifests[0];ArchivePack result;
    result.pack=parseManifest(archiveBytes(selectedArchive,entry.index,8*1024*1024));result.archive=selectedArchive;
    const auto slash=entry.path.lastIndexOf('/');result.base=slash<0?QString{}:entry.path.left(slash+1);
    QMap<QString,Override> selected;
    for(const auto& e:entries){
        if(cancel)fail(ImportError::Cancelled);
        if(e.directory)continue;
        int layer=0;QString relative;
        for(const auto& dir:{QString("overrides/"),QString("client-overrides/")}){
            ++layer;const auto prefix=result.base+dir;if(e.path.startsWith(prefix)){relative=e.path.mid(prefix.size());break;}
        }
        if(relative.isEmpty())continue;
        if(e.size>1024ULL*1024*1024)fail(ImportError::InvalidArchive,"override-size");
        const auto key=relative.toCaseFolded();const auto old=selected.constFind(key);
        if(old!=selected.cend()&&old->path!=relative)fail(ImportError::UnsafePath,"case-collision");
        if(old==selected.cend()||old->layer<layer)selected.insert(key,{e.index,relative,e.size,layer});
    }
    std::vector<std::string> all;QMap<QString,QString> destinations;
    for(const auto& f:result.pack.files)destinations.insert(QString::fromStdString(f.path).toCaseFolded(),QString::fromStdString(f.path));
    for(const auto& o:selected){
        const auto old=destinations.constFind(o.path.toCaseFolded());
        if(old!=destinations.cend()&&old.value()!=o.path)fail(ImportError::UnsafePath,"case-collision");
        destinations.insert(o.path.toCaseFolded(),o.path);result.overrides.push_back(o);
    }
    for(auto it=destinations.begin();it!=destinations.end();++it)all.push_back(it.key().toStdString());
    if(core::packPathsConflict(all))fail(ImportError::UnsafePath,"file-directory-conflict");
    return result;
}
}
