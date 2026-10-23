#include "modpack_io.h"
#include <hustmcl/services/java_probe.h>
#include <QBuffer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace hustmcl::services::pack {
using application::ImportError;
[[noreturn]] void stop(ImportError error,QString context){throw application::ImportFailure{error,std::move(context)};}
void checkCancel(const std::atomic_bool& cancel){if(cancel)stop(ImportError::Cancelled);}
namespace {
DWORD attributes(const QString& path){
    auto native=QDir::toNativeSeparators(path);
    if(!native.startsWith("\\\\?\\"))native.prepend("\\\\?\\");
    return GetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()));
}
}
bool safeExisting(QString path){
    path=QDir::cleanPath(path);
    if(!QRegularExpression("^[A-Za-z]:/").match(path).hasMatch())return false;
    for(;;){const auto a=attributes(path);if(a==INVALID_FILE_ATTRIBUTES||(a&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        const auto parent=QFileInfo(path).absolutePath();if(parent==path)return true;path=parent;}
}
void mkdir(const QString& path){
    const auto clean=QDir::cleanPath(path);
    if(!QRegularExpression("^[A-Za-z]:/").match(clean).hasMatch())stop(ImportError::UnsafePath);
    const auto a=attributes(clean);
    if(a!=INVALID_FILE_ATTRIBUTES){if(!safeExisting(clean)||!(a&FILE_ATTRIBUTE_DIRECTORY))stop(ImportError::UnsafePath);return;}
    const auto parent=QFileInfo(clean).absolutePath();if(parent==clean)stop(ImportError::Storage);
    mkdir(parent);
    if(!QDir(parent).mkdir(QFileInfo(clean).fileName())||!safeExisting(clean))stop(ImportError::Storage);
}
void write(const QString& target,const QByteArray& bytes){
    mkdir(QFileInfo(target).absolutePath());
    if(QFileInfo::exists(target)&&!safeExisting(target))stop(ImportError::UnsafePath);
    QSaveFile f(target);if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit())stop(ImportError::Storage);
}
void copy(const QString& source,const QString& target,const std::atomic_bool& cancel){
    checkCancel(cancel);if(!safeExisting(source)||!QFileInfo(source).isFile())stop(ImportError::UnsafePath);
    mkdir(QFileInfo(target).absolutePath());
    if(QFileInfo::exists(target)&&!safeExisting(target))stop(ImportError::UnsafePath);
    QFile input(source);QSaveFile output(target);
    if(!input.open(QIODevice::ReadOnly)||!output.open(QIODevice::WriteOnly))stop(ImportError::Storage);
    while(!input.atEnd()){
        checkCancel(cancel);const auto b=input.read(65536);
        if((b.isEmpty()&&input.error()!=QFileDevice::NoError)||output.write(b)!=b.size())stop(ImportError::Storage);
    }
    checkCancel(cancel);if(!output.commit())stop(ImportError::Storage);
}
QByteArray hash(const QString& path,QCryptographicHash::Algorithm algorithm,const std::atomic_bool& cancel){
    if(!safeExisting(path)||!QFileInfo(path).isFile())stop(ImportError::UnsafePath);
    QFile input(path);if(!input.open(QIODevice::ReadOnly))stop(ImportError::Storage);QCryptographicHash h(algorithm);
    while(!input.atEnd()){checkCancel(cancel);const auto b=input.read(65536);if(b.isEmpty()&&input.error()!=QFileDevice::NoError)stop(ImportError::Storage);h.addData(b);}
    return h.result().toHex();
}
core::JavaRuntime probeJava(const QString& path,const std::atomic_bool& cancel){
    checkCancel(cancel);if(!safeExisting(path))stop(ImportError::JavaUnavailable);
    services::QtJavaProbe probe;QEventLoop loop;QTimer poll;std::optional<application::JavaProbeResult> result;
    poll.setInterval(50);QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(cancel)probe.cancel("pack-java");});
    probe.probe("pack-java",path.toStdWString(),[&](auto r){result=std::move(r);loop.quit();});
    if(!result){poll.start();loop.exec();}poll.stop();checkCancel(cancel);
    if(!result||!std::holds_alternative<core::JavaRuntime>(*result))stop(ImportError::JavaUnavailable);
    return std::get<core::JavaRuntime>(*result);
}
Network::Network(const std::atomic_bool& cancel,bool loopback,int timeout):cancelled_(cancel),loopback_(loopback),timeoutMs_(timeout){}
bool Network::allowed(const QUrl& u)const{
    if(loopback_&&u.scheme()=="http"&&u.host()=="127.0.0.1"&&u.userInfo().isEmpty()&&!u.hasFragment())return true;
    static const QStringList hosts{"piston-meta.mojang.com","piston-data.mojang.com","launchermeta.mojang.com",
        "resources.download.minecraft.net","libraries.minecraft.net","meta.fabricmc.net","maven.fabricmc.net",
        "repo.maven.apache.org","maven.minecraftforge.net","maven.neoforged.net","api.adoptium.net",
        "github.com","raw.githubusercontent.com","objects.githubusercontent.com","release-assets.githubusercontent.com",
        "gitlab.com","cdn.modrinth.com","edge.forgecdn.net","media.forgecdn.net","mediafilez.forgecdn.net"};
    return u.isValid()&&u.scheme()=="https"&&u.port(-1)==-1&&u.userInfo().isEmpty()&&!u.hasFragment()&&hosts.contains(u.host());
}
QUrl Network::request(QUrl url,QIODevice* output,quint64 maximum,bool head){
    for(int redirect=0;redirect<=5;++redirect){
        checkCancel(cancelled_);if(!allowed(url)||url.toString().contains('\\'))stop(ImportError::InvalidManifest,"download-host");
        QNetworkRequest req(url);req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
        req.setRawHeader("User-Agent","HustMCL/0.1.0 (+https://github.com/qiu26258-beep/HustMCL)");req.setRawHeader("Accept-Encoding","identity");
        std::unique_ptr<QNetworkReply> owned(head?manager_.head(req):manager_.get(req));auto* reply=owned.get();reply->setReadBufferSize(65536);
        QEventLoop loop;QTimer deadline,poll;deadline.setSingleShot(true);poll.setInterval(50);bool timed=false,exceeded=false;quint64 received=0;
        auto drain=[&]{
            if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()/100!=2)return;
            while(reply->bytesAvailable()){
                const auto b=reply->read(65536);received+=static_cast<quint64>(b.size());
                if(received>maximum||(output&&output->write(b)!=b.size())){exceeded=true;reply->abort();return;}
            }
        };
        QObject::connect(reply,&QNetworkReply::readyRead,&loop,drain);
        QObject::connect(reply,&QNetworkReply::metaDataChanged,&loop,[&]{if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()/100==3)reply->abort();});
        QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
        QObject::connect(&deadline,&QTimer::timeout,&loop,[&]{timed=true;reply->abort();});
        QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(cancelled_)reply->abort();});
        deadline.start(timeoutMs_);poll.start();if(!reply->isFinished())loop.exec();drain();deadline.stop();poll.stop();checkCancel(cancelled_);
        const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status/100==3){url=url.resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());continue;}
        if(exceeded)stop(ImportError::Integrity,"response-size");
        if(timed||reply->error()!=QNetworkReply::NoError||status/100!=2)stop(ImportError::Network,"HTTP "+QString::number(status));
        return url;
    }
    stop(ImportError::Network,"redirect-limit");
}
QByteArray Network::bytes(const QString& url,quint64 maximum){QBuffer b;b.open(QIODevice::ReadWrite);request(QUrl(url,QUrl::StrictMode),&b,maximum,false);return b.data();}
void Network::file(const QString& url,const QString& path,quint64 maximum){
    mkdir(QFileInfo(path).absolutePath());if(QFileInfo::exists(path)&&!safeExisting(path))stop(ImportError::UnsafePath);
    QSaveFile f(path);if(!f.open(QIODevice::WriteOnly))stop(ImportError::Storage);
    request(QUrl(url,QUrl::StrictMode),&f,maximum,false);checkCancel(cancelled_);if(!f.commit())stop(ImportError::Storage);
}
QString Network::resolve(const QString& url){return request(QUrl(url,QUrl::StrictMode),nullptr,0,true).toString(QUrl::FullyEncoded);}
}
