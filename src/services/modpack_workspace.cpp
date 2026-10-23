#include <hustmcl/services/modpack_workspace.h>
#include <hustmcl/services/asset_index.h>
#include <hustmcl/services/file_acquirer.h>
#include <hustmcl/services/local_game.h>
#include <hustmcl/services/version_metadata.h>
#include <hustmcl/application/batch_acquisition.h>
#include "modpack_archive.h"
#include "modpack_io.h"
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QThreadPool>
#include <QTimer>
#include <algorithm>

namespace hustmcl::services {
using namespace application;
namespace {
QByteArray read(const QString& path,qint64 maximum=16*1024*1024){
    if(!pack::safeExisting(path))pack::stop(ImportError::UnsafePath);
    QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()<0||f.size()>maximum)pack::stop(ImportError::Storage);
    const auto b=f.readAll();if(f.error()!=QFileDevice::NoError)pack::stop(ImportError::Storage);return b;
}
QJsonObject object(const QByteArray& bytes){
    QJsonParseError e;const auto d=QJsonDocument::fromJson(bytes,&e);
    if(e.error!=QJsonParseError::NoError||!d.isObject())pack::stop(ImportError::InvalidManifest);
    return d.object();
}
bool hex(const QByteArray& value,int length){return value.size()==length&&QRegularExpression("^[0-9a-f]+$").match(QString::fromLatin1(value)).hasMatch();}
QString relativeName(const std::filesystem::path& p){return QString::fromStdWString(p.generic_wstring());}
QString acquired(const QString& root,const core::DownloadDescriptor& d,const std::atomic_bool& cancel,const QtModpackWorkspace::Options& options){
    pack::checkCancel(cancel);QtFileAcquirer acquirer({options.timeoutMs,options.allowLoopbackHttp});QEventLoop loop;QTimer poll;
    std::optional<FileResult> result;poll.setInterval(50);
    QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(cancel)acquirer.cancel("pack-file");});
    acquirer.acquire("pack-file",root,d,[&](auto r){result=std::move(r);loop.quit();});
    if(!result){poll.start();loop.exec();}poll.stop();pack::checkCancel(cancel);
    if(!result||result->error!=FileError::None){
        const auto e=result?result->error:FileError::Network;
        pack::stop(e==FileError::Integrity?ImportError::Integrity:e==FileError::Locked?ImportError::Busy:
            e==FileError::UnsafePath?ImportError::UnsafePath:e==FileError::Storage?ImportError::Storage:ImportError::Network);
    }
    return result->cachePath;
}
void batch(const QString& root,const QString& payload,const core::InstallFilePlan& plan,const std::atomic_bool& cancel,
           const QtModpackWorkspace::Options& options,const ModpackWorkspace::Progress& progress){
    if(plan.files.empty())return;
    QtFileAcquirer acquirer({options.timeoutMs,options.allowLoopbackHttp});BatchAcquisition acquisition(acquirer,nullptr,4);
    QEventLoop loop;QTimer poll;poll.setInterval(50);
    QObject::connect(&acquisition,&BatchAcquisition::changed,&loop,[&](const auto& s){
        progress({s.completedFiles,s.totalFiles,s.verifiedBytes,s.totalBytes});
        if(s.phase!=AcquisitionPhase::Acquiring&&s.phase!=AcquisitionPhase::Cancelling)loop.quit();
    });
    QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(cancel)acquisition.cancel(acquisition.snapshot().taskId);});
    if(!acquisition.start(root,plan))pack::stop(ImportError::Busy);
    if(acquisition.snapshot().phase==AcquisitionPhase::Acquiring){poll.start();loop.exec();}poll.stop();pack::checkCancel(cancel);
    const auto s=acquisition.snapshot();
    if(s.phase!=AcquisitionPhase::Cached)pack::stop(s.error==FileError::Integrity?ImportError::Integrity:s.error==FileError::Locked?ImportError::Busy:ImportError::Network);
    const auto paths=acquisition.cachePaths();if(static_cast<std::size_t>(paths.size())!=plan.files.size())pack::stop(ImportError::Integrity);
    for(std::size_t i=0;i<plan.files.size();++i){
        const auto target=QDir(payload).filePath(relativeName(plan.files[i].relativePath));pack::copy(paths[static_cast<qsizetype>(i)],target,cancel);
        if(pack::hash(target,QCryptographicHash::Sha1,cancel).toStdString()!=plan.files[i].download.sha1)pack::stop(ImportError::Integrity);
    }
}
QString mavenPath(const QString& name){
    const auto p=name.split(':');if(p.size()<3||p.size()>4)pack::stop(ImportError::InvalidManifest,"maven-coordinate");
    for(const auto& s:p)if(!QRegularExpression("^[A-Za-z0-9_.+-]+$").match(s).hasMatch())pack::stop(ImportError::InvalidManifest,"maven-coordinate");
    auto group=p[0];group.replace('.','/');return group+'/'+p[1]+'/'+p[2]+'/'+p[1]+'-'+p[2]+(p.size()==4?'-'+p[3]:QString{})+".jar";
}
QString libraryKey(const QString& name){const auto p=name.split(':');return p.value(0)+':'+p.value(1)+':'+p.value(3);}
void fabric(QJsonObject& version,const core::Modpack& pack,const QString& payload,pack::Network& net,const std::atomic_bool& cancel,
            const QtModpackWorkspace::Options& options){
    const auto profile=object(net.bytes(options.fabricMetaBase+QString::fromStdString(pack.minecraft)+'/'+QString::fromStdString(pack.loaderVersion)+"/profile/json"));
    if(!profile.value("mainClass").isString()||!profile.value("libraries").isArray()
        ||profile.value("inheritsFrom").toString()!=QString::fromStdString(pack.minecraft))pack::stop(ImportError::InvalidManifest,"fabric-profile");
    QJsonArray added;QSet<QString> replacing;int index=0;
    for(const auto& item:profile.value("libraries").toArray()){
        pack::checkCancel(cancel);const auto lib=item.toObject();const auto name=lib.value("name").toString();const auto path=mavenPath(name);
        const auto repo=lib.value("url").toString();QUrl u(repo);if(!net.allowed(u))pack::stop(ImportError::InvalidManifest,"fabric-repository");
        const auto address=repo+(repo.endsWith('/')?QString{}:QString("/"))+path;
        auto sha1=net.bytes(address+".sha1",1024).trimmed().toLower().left(40);if(!hex(sha1,40))pack::stop(ImportError::InvalidManifest,"fabric-hash");
        const auto destination=QDir(payload).filePath("libraries/"+path);
        net.file(address,destination,64*1024*1024);
        if(pack::hash(destination,QCryptographicHash::Sha1,cancel)!=sha1)pack::stop(ImportError::Integrity,"fabric-library");
        const QJsonObject artifact{{"path",path},{"url",address},{"sha1",QString::fromLatin1(sha1)},{"size",QFileInfo(destination).size()}};
        added.append(QJsonObject{{"name",name},{"downloads",QJsonObject{{"artifact",artifact}}}});replacing.insert(libraryKey(name));++index;
        if(index>128)pack::stop(ImportError::InvalidManifest,"fabric-library-limit");
    }
    QJsonArray libraries;for(const auto& v:version.value("libraries").toArray())if(!replacing.contains(libraryKey(v.toObject().value("name").toString())))libraries.append(v);
    for(const auto& l:added)libraries.append(l);
    version.insert("libraries",libraries);version.insert("mainClass",profile.value("mainClass"));
    auto arguments=version.value("arguments").toObject();const auto patch=profile.value("arguments").toObject();
    for(const auto* key:{"jvm","game"}){auto a=arguments.value(key).toArray();for(const auto& v:patch.value(key).toArray())a.append(v);arguments.insert(key,a);}
    version.insert("arguments",arguments);
}
QString javaRuntime(const QString& bundled,const QString& payload,int major,pack::Network& net,const std::atomic_bool& cancel,
                    const QtModpackWorkspace::Options& options){
    try{const auto existing=pack::probeJava(bundled,cancel);if(existing.majorVersion==major&&existing.architecture==core::RuntimeArchitecture::X64)return "@bundled";}
    catch(const ImportFailure& f){if(f.code==ImportError::Cancelled)throw;}
    const auto bytes=net.bytes(options.javaApiBase+QString::number(major)+"/hotspot?architecture=x64&image_type=jre&os=windows&vendor=eclipse");
    QJsonParseError e;const auto doc=QJsonDocument::fromJson(bytes,&e);
    if(e.error!=QJsonParseError::NoError||!doc.isArray()||doc.array().isEmpty())pack::stop(ImportError::JavaUnavailable);
    const auto p=doc.array()[0].toObject().value("binary").toObject().value("package").toObject();
    const auto checksum=p.value("checksum").toString().toLatin1().toLower();const auto size=p.value("size").toInteger(-1);
    if(!hex(checksum,64)||size<1||size>512LL*1024*1024)pack::stop(ImportError::JavaUnavailable);
    const auto archive=QDir(payload).absoluteFilePath("../java.zip");net.file(p.value("link").toString(),archive,static_cast<quint64>(size));
    if(QFileInfo(archive).size()!=size||pack::hash(archive,QCryptographicHash::Sha256,cancel)!=checksum)pack::stop(ImportError::Integrity,"java-archive");
    const auto entries=pack::archiveEntries(archive);QString executable;
    for(const auto& entry:entries){
        pack::checkCancel(cancel);if(entry.directory)continue;
        const auto dest=QDir(payload).filePath("runtime/"+entry.path);pack::mkdir(QFileInfo(dest).absolutePath());
        pack::extractEntry(archive,entry.index,dest,cancel);
        if(entry.path.endsWith("/bin/java.exe")){if(!executable.isEmpty())pack::stop(ImportError::JavaUnavailable);executable="runtime/"+entry.path;}
    }
    if(executable.isEmpty())pack::stop(ImportError::JavaUnavailable);
    const auto runtime=pack::probeJava(QDir(payload).filePath(executable),cancel);
    if(runtime.majorVersion!=major||runtime.architecture!=core::RuntimeArchitecture::X64)pack::stop(ImportError::JavaUnavailable);
    return executable;
}
bool validId(const QString& id){return QRegularExpression("^pack-[0-9a-f]{64}$").match(id).hasMatch();}
InstalledInstance instanceFrom(const QString& root,const QString& bundled){
    const auto json=object(read(QDir(root).filePath("instance.json")));
    const auto id=json.value("id").toString(),name=json.value("name").toString(),ver=json.value("versionId").toString(),java=json.value("java").toString();
    if(json.value("schema")!=1||!validId(id)||QFileInfo(root).fileName()!=id||name.isEmpty()||name.size()>512
        ||!core::validPackVersion(ver.toStdString())||!json.value("files").isArray())pack::stop(ImportError::InvalidManifest,"instance-record");
    if(java!="@bundled"&&(!core::validPackPath(java.toStdString())||!java.startsWith("runtime/")||!java.endsWith("/bin/java.exe")))pack::stop(ImportError::UnsafePath);
    return {id,name,json.value("versionLabel").toString(),root,ver,java=="@bundled"?bundled:QDir(root).filePath(java)};
}
}
struct QtModpackWorkspace::Impl {
    struct Job {
        QString task,stage,payload,id,java;
        pack::ArchivePack archive;
        std::unique_ptr<QLockFile> lock;
        std::atomic_bool cancelled=false;
        QJsonArray files;
    };
    QtModpackWorkspace* owner;
    QString root,bundled;
    Options options;
    QThreadPool workers;
    std::shared_ptr<Job> job;
    Impl(QtModpackWorkspace* o,QString r,QString b,Options opt):owner(o),root(QDir::cleanPath(r)),bundled(std::move(b)),options(std::move(opt)){workers.setMaxThreadCount(1);}
    ~Impl(){if(job)job->cancelled=true;workers.waitForDone();}
    template<class T,class Work>void run(std::shared_ptr<Job> j,Work work,std::function<void(ImportResult<T>)> complete){
        const QPointer<QtModpackWorkspace> guard(owner);
        workers.start([guard,j=std::move(j),work=std::move(work),complete=std::move(complete)]()mutable{
            auto perform=[&]()->ImportResult<T>{try{return work(*j);}catch(const ImportFailure& f){return f;}catch(...){return ImportFailure{ImportError::Storage,"operation"};}};
            auto result=perform();if(guard)QMetaObject::invokeMethod(guard.data(),[guard,complete=std::move(complete),result=std::move(result)]()mutable{if(guard)complete(std::move(result));},Qt::QueuedConnection);
        });
    }
    Progress progress(Progress callback){
        const QPointer<QtModpackWorkspace> guard(owner);
        return [guard,callback=std::move(callback)](ImportProgress p){if(guard)QMetaObject::invokeMethod(guard.data(),[guard,callback,p]{if(guard)callback(p);},Qt::QueuedConnection);};
    }
};
QtModpackWorkspace::QtModpackWorkspace(QString root,QString java,Options options,QObject* parent)
    :QObject(parent),impl_(std::make_unique<Impl>(this,std::move(root),std::move(java),std::move(options))){}
QtModpackWorkspace::~QtModpackWorkspace()=default;
void QtModpackWorkspace::inspect(const QString& task,const QString& archive,std::function<void(ImportResult<core::Modpack>)> complete){
    if(impl_->job){complete(ImportFailure{ImportError::Busy,{}});return;}
    auto j=std::make_shared<Impl::Job>();j->task=task;impl_->job=j;const auto root=impl_->root;
    impl_->run<core::Modpack>(j,[root,archive](auto& job){
        if(!QDir::isAbsolutePath(archive)||!pack::safeExisting(archive)||!QFileInfo(archive).isFile()
            ||(!archive.endsWith(".zip",Qt::CaseInsensitive)&&!archive.endsWith(".mrpack",Qt::CaseInsensitive)))pack::stop(ImportError::InvalidArchive);
        if(QFileInfo(archive).size()>8LL*1024*1024*1024)pack::stop(ImportError::InvalidArchive,"archive-size");
        pack::mkdir(root);const auto lockPath=QDir(root).filePath(".modpack-import.lock");
        if(QFileInfo::exists(lockPath)&&!pack::safeExisting(lockPath))pack::stop(ImportError::UnsafePath);
        job.lock=std::make_unique<QLockFile>(lockPath);job.lock->setStaleLockTime(0);if(!job.lock->tryLock(0))pack::stop(ImportError::Busy);
        job.stage=QDir(root).filePath("modpack-staging/"+job.task);job.payload=QDir(job.stage).filePath("payload");pack::mkdir(job.payload);
        pack::write(QDir(job.stage).filePath("owner.json"),QJsonDocument(QJsonObject{{"schema",1},{"task",job.task}}).toJson());
        const auto snapshot=QDir(job.stage).filePath("source.zip");pack::copy(archive,snapshot,job.cancelled);
        job.id="pack-"+QString::fromLatin1(pack::hash(snapshot,QCryptographicHash::Sha256,job.cancelled));
        if(QFileInfo::exists(QDir(root).filePath("instances/"+job.id)))pack::stop(ImportError::Conflict,"already-installed");
        job.archive=pack::inspectArchive(snapshot,QDir(job.stage).filePath("inner.mrpack"),job.cancelled);
        return job.archive.pack;
    },std::move(complete));
}
void QtModpackWorkspace::prepareGame(const QString& task,Progress p,Done complete){
    const auto j=impl_->job;if(!j||j->task!=task){complete(ImportFailure{ImportError::Busy,{}});return;}
    const auto root=impl_->root,bundled=impl_->bundled;const auto options=impl_->options;
    impl_->run<std::monostate>(j,[root,bundled,options,p=impl_->progress(std::move(p))](auto& job){
        pack::checkCancel(job.cancelled);const auto& pack=job.archive.pack;
        if(pack.loader!=core::PackLoader::Vanilla&&pack.loader!=core::PackLoader::Fabric)pack::stop(ImportError::UnsupportedLoader);
        pack::Network net(job.cancelled,options.allowLoopbackHttp,options.timeoutMs);
        const auto manifest=object(net.bytes(options.versionManifestUrl));QJsonObject selected;
        for(const auto& v:manifest.value("versions").toArray())if(v.toObject().value("id").toString()==QString::fromStdString(pack.minecraft)){selected=v.toObject();break;}
        const auto expected=selected.value("sha1").toString().toLatin1();if(!hex(expected,40))pack::stop(ImportError::InvalidManifest,"minecraft-version");
        const auto bytes=net.bytes(selected.value("url").toString(),4*1024*1024);
        if(QCryptographicHash::hash(bytes,QCryptographicHash::Sha1).toHex()!=expected)pack::stop(ImportError::Integrity,"minecraft-metadata");
        auto json=object(bytes);auto parsed=parseVersionMetadata(bytes);
        if(!std::holds_alternative<core::VersionMetadata>(parsed)||std::get<core::VersionMetadata>(parsed).id!=pack.minecraft)pack::stop(ImportError::InvalidManifest,"minecraft-metadata");
        const auto indexDescriptor=std::get<core::VersionMetadata>(parsed).assetIndex;
        const auto indexFile=acquired(root,indexDescriptor,job.cancelled,options);const auto indexBytes=read(indexFile,core::MaximumAssetIndexBytes);
        if(pack.loader==core::PackLoader::Fabric)fabric(json,pack,job.payload,net,job.cancelled,options);
        const auto normalized=QJsonDocument(json).toJson(QJsonDocument::Compact);parsed=parseVersionMetadata(normalized);
        if(!std::holds_alternative<core::VersionMetadata>(parsed))pack::stop(ImportError::InvalidManifest,"loader-metadata");
        const auto& version=std::get<core::VersionMetadata>(parsed);auto index=parseAssetIndex(indexBytes,indexDescriptor);
        if(!std::holds_alternative<core::AssetIndex>(index))pack::stop(ImportError::Integrity,"asset-index");
        auto planned=core::buildInstallFilePlan(version,std::get<core::AssetIndex>(index));
        if(!std::holds_alternative<core::InstallFilePlan>(planned))pack::stop(ImportError::InvalidManifest,"game-plan");
        auto plan=std::get<core::InstallFilePlan>(std::move(planned));core::InstallFilePlan missing=plan;missing.files.clear();missing.totalBytes=0;
        for(const auto& f:plan.files){
            const auto local=QDir(job.payload).filePath(relativeName(f.relativePath));
            if(QFileInfo::exists(local)&&QFileInfo(local).size()==static_cast<qint64>(f.download.size)
                &&pack::hash(local,QCryptographicHash::Sha1,job.cancelled).toStdString()==f.download.sha1)continue;
            missing.files.push_back(f);missing.totalBytes+=f.download.size;
        }
        batch(root,job.payload,missing,job.cancelled,options,p);
        pack::write(QDir(job.payload).filePath("versions/"+QString::fromStdString(version.id)+'/'+QString::fromStdString(version.id)+".json"),normalized);
        p({});job.java=javaRuntime(bundled,job.payload,version.java.majorVersion,net,job.cancelled,options);
        if(!prepareLocalGameDirectories(job.payload))pack::stop(ImportError::Storage);
        return std::monostate{};
    },std::move(complete));
}
void QtModpackWorkspace::installFiles(const QString& task,Progress p,Done complete){
    const auto j=impl_->job;if(!j||j->task!=task){complete(ImportFailure{ImportError::Busy,{}});return;}
    const auto root=impl_->root;const auto options=impl_->options;
    impl_->run<std::monostate>(j,[root,options,p=impl_->progress(std::move(p))](auto& job){
        pack::Network net(job.cancelled,options.allowLoopbackHttp,options.timeoutMs);ImportProgress state;
        state.totalFiles=job.archive.pack.files.size()+job.archive.overrides.size();
        for(const auto& f:job.archive.pack.files)state.totalBytes+=f.size;
        for(const auto& f:job.archive.overrides)state.totalBytes+=f.size;
        QSet<QString> destinations;
        for(const auto& f:job.archive.pack.files){
            pack::checkCancel(job.cancelled);QString cache;std::optional<ImportFailure> last;
            for(const auto& u:f.urls){
                try{
                    auto url=QString::fromStdString(u);
                    if(!QFileInfo::exists(QDir(root).filePath("cache/sha1/"+QString::fromStdString(f.sha1))))url=net.resolve(url);
                    cache=acquired(root,{url.toStdString(),f.sha1,f.size},job.cancelled,options);
                    if(pack::hash(cache,QCryptographicHash::Sha512,job.cancelled).toStdString()!=f.sha512)pack::stop(ImportError::Integrity,"pack-file");
                    break;
                }catch(const ImportFailure& e){if(e.code==ImportError::Cancelled||e.code==ImportError::UnsafePath||e.code==ImportError::Busy)throw;last=e;cache.clear();}
            }
            if(cache.isEmpty())throw last.value_or(ImportFailure{ImportError::Network,{}});
            const auto path=QString::fromStdString(f.path);pack::copy(cache,QDir(job.payload).filePath("game/"+path),job.cancelled);destinations.insert(path);
            ++state.completedFiles;state.completedBytes+=f.size;p(state);
        }
        for(const auto& f:job.archive.overrides){
            pack::checkCancel(job.cancelled);const auto target=QDir(job.payload).filePath("game/"+f.path);pack::mkdir(QFileInfo(target).absolutePath());
            pack::extractEntry(job.archive.archive,f.index,target,job.cancelled);destinations.insert(f.path);
            ++state.completedFiles;state.completedBytes+=f.size;p(state);
        }
        job.files={};for(const auto& path:destinations){const auto file=QDir(job.payload).filePath("game/"+path);
            job.files.append(QJsonObject{{"path",path},{"size",QFileInfo(file).size()},{"sha512",QString::fromLatin1(pack::hash(file,QCryptographicHash::Sha512,job.cancelled))}});}
        return std::monostate{};
    },std::move(complete));
}
void QtModpackWorkspace::commit(const QString& task,std::function<void(ImportResult<InstalledInstance>)> complete){
    const auto j=impl_->job;if(!j||j->task!=task){complete(ImportFailure{ImportError::Busy,{}});return;}
    const auto root=impl_->root,bundled=impl_->bundled;
    impl_->run<InstalledInstance>(j,[root,bundled](auto& job){
        pack::checkCancel(job.cancelled);const auto& p=job.archive.pack;const auto version=QString::fromStdString(p.minecraft);
        if(!std::holds_alternative<LocalGame>(verifyLocalGame(job.payload,version)))pack::stop(ImportError::Integrity,"installed-game");
        for(const auto& item:job.files){const auto record=item.toObject();const auto path=QDir(job.payload).filePath("game/"+record.value("path").toString());
            if(QFileInfo(path).size()!=record.value("size").toInteger(-1)||pack::hash(path,QCryptographicHash::Sha512,job.cancelled)!=record.value("sha512").toString().toLatin1())pack::stop(ImportError::Integrity,"installed-pack");}
        const auto label=version+(p.loader==core::PackLoader::Fabric?" · Fabric "+QString::fromStdString(p.loaderVersion):" · Vanilla");
        const QJsonObject record{{"schema",1},{"id",job.id},{"name",QString::fromStdString(p.name)},{"packVersion",QString::fromStdString(p.version)},
            {"versionId",version},{"versionLabel",label},{"java",job.java},{"files",job.files}};
        pack::write(QDir(job.payload).filePath("instance.json"),QJsonDocument(record).toJson());
        const auto instances=QDir(root).filePath("instances");pack::mkdir(instances);const auto target=QDir(instances).filePath(job.id);
        if(QFileInfo::exists(target))pack::stop(ImportError::Conflict);
        pack::checkCancel(job.cancelled);
        if(!QDir().rename(job.payload,target))pack::stop(ImportError::Storage,"publish-instance");
        // The rename is the completion point. No fallible work after it.
        return InstalledInstance{job.id,QString::fromStdString(p.name),label,target,version,job.java=="@bundled"?bundled:QDir(target).filePath(job.java)};
    },std::move(complete));
}
void QtModpackWorkspace::cancel(const QString& task){if(impl_->job&&impl_->job->task==task)impl_->job->cancelled=true;}
void QtModpackWorkspace::release(const QString& task){if(impl_->job&&impl_->job->task==task)impl_->job.reset();}
void QtModpackWorkspace::listInstalled(std::function<void(ImportResult<std::vector<InstalledInstance>>)> complete){
    const auto root=impl_->root,bundled=impl_->bundled;auto placeholder=std::make_shared<Impl::Job>();
    impl_->run<std::vector<InstalledInstance>>(placeholder,[root,bundled](auto&){
        std::vector<InstalledInstance> result;const auto path=QDir(root).filePath("instances");if(!QFileInfo::exists(path))return result;
        if(!pack::safeExisting(path))pack::stop(ImportError::UnsafePath);
        const auto entries=QDir(path).entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        if(entries.size()>1000)pack::stop(ImportError::Storage,"instance-limit");
        for(const auto& name:entries){if(!validId(name))continue;try{result.push_back(instanceFrom(QDir(path).filePath(name),bundled));}catch(const ImportFailure&){/* Preserve unknown/corrupt records. */}}
        return result;
    },std::move(complete));
}
bool verifyImportedPackFiles(const QString& root,bool requireRecord){
    if(!QFileInfo::exists(QDir(root).filePath("instance.json")))return !requireRecord;
    try{
        const auto json=object(read(QDir(root).filePath("instance.json")));const auto files=json.value("files").toArray();std::atomic_bool cancelled=false;
        if(json.value("schema")!=1||files.size()>50000||!json.value("files").isArray())return false;
        std::vector<std::string> paths;
        for(const auto& item:files){const auto r=item.toObject();const auto p=r.value("path").toString();if(!core::validPackPath(p.toStdString()))return false;paths.push_back(p.toCaseFolded().toStdString());
            // Configs and worlds legitimately change after play; installed mod JARs remain verified.
            if(!p.startsWith("mods/",Qt::CaseInsensitive)||!p.endsWith(".jar",Qt::CaseInsensitive))continue;
            const auto f=QDir(root).filePath("game/"+p);const auto sha=r.value("sha512").toString().toLatin1();
            if(!hex(sha,128)||QFileInfo(f).size()!=r.value("size").toInteger(-1)||pack::hash(f,QCryptographicHash::Sha512,cancelled)!=sha)return false;
        }
        return !core::packPathsConflict(paths);
    }catch(...){return false;}
}
}
