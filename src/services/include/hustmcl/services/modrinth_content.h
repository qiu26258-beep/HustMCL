#pragma once
#include <hustmcl/application/content_market.h>
#include <QUrl>
#include <map>
#include <memory>
class QNetworkAccessManager;
class QNetworkReply;

namespace hustmcl::services {
application::ContentResult<core::ContentPage> parseModrinthSearch(const QByteArray&);
application::ContentResult<std::vector<core::ContentVersion>> parseModrinthVersions(const QByteArray&, const std::string& projectId);
class ModrinthContentSource final : public QObject, public application::ContentSource {
public:
    struct Options { QUrl base = QUrl("https://api.modrinth.com/v2/"); int timeoutMs = 15000; bool allowLoopbackHttp = false; };
    explicit ModrinthContentSource(Options options, QObject* parent = nullptr);
    ~ModrinthContentSource() override;
    void search(const QString&, const core::ContentQuery&, std::function<void(application::ContentResult<core::ContentPage>)>) override;
    void versions(const QString&, const std::string&, const core::ContentQuery&,
                  std::function<void(application::ContentResult<std::vector<core::ContentVersion>>)>) override;
    void cancel(const QString&) override;
private:
    void get(const QString&, const QUrl&, std::function<void(application::ContentResult<QByteArray>)>);
    Options options_;
    std::unique_ptr<QNetworkAccessManager> network_;
    std::map<QString, QNetworkReply*> replies_;
};
}
