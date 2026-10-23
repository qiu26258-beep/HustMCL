#pragma once
#include <hustmcl/application/modpack_import.h>
#include <hustmcl/core/java_runtime.h>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QString>
#include <atomic>

namespace hustmcl::services::pack {
[[noreturn]] void stop(application::ImportError,QString context={});
void checkCancel(const std::atomic_bool&);
bool safeExisting(QString path);
void mkdir(const QString&);
void write(const QString&,const QByteArray&);
void copy(const QString&,const QString&,const std::atomic_bool&);
QByteArray hash(const QString&,QCryptographicHash::Algorithm,const std::atomic_bool&);
core::JavaRuntime probeJava(const QString&,const std::atomic_bool&);
class Network {
public:
    Network(const std::atomic_bool& cancelled,bool loopback=false,int timeoutMs=30000);
    QByteArray bytes(const QString& url,quint64 maximum=8*1024*1024);
    void file(const QString& url,const QString& destination,quint64 maximum);
    QString resolve(const QString& url);
    bool allowed(const QUrl&)const;
private:
    QUrl request(QUrl url,QIODevice* output,quint64 maximum,bool head);
    const std::atomic_bool& cancelled_;
    bool loopback_;
    int timeoutMs_;
    QNetworkAccessManager manager_;
};
}
