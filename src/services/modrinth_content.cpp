#include <hustmcl/services/modrinth_content.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QTimer>
#include <QUrlQuery>
#include <cmath>
#include <memory>

namespace hustmcl::services {
using namespace application;
namespace {
constexpr qsizetype MaxBody = 4 * 1024 * 1024;
bool text(const QJsonObject& o, const char* key, int maximum, bool empty = false) {
    return o.value(key).isString() && o.value(key).toString().size() <= maximum && (empty || !o.value(key).toString().isEmpty());
}
std::string s(const QJsonObject& o, const char* key) { return o.value(key).toString().toUtf8().toStdString(); }
bool integer(const QJsonValue& v, double maximum) { return v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= 0 && v.toDouble() <= maximum && std::floor(v.toDouble()) == v.toDouble(); }
bool strings(const QJsonValue& value, std::vector<std::string>& result) {
    if (!value.isArray() || value.toArray().size() > 500) return false;
    for (const auto& v : value.toArray()) { if (!v.isString() || v.toString().size() > 128) return false; result.push_back(v.toString().toUtf8().toStdString()); }
    return true;
}
QByteArray array(const std::string& value) { return QJsonDocument(QJsonArray{QString::fromStdString(value)}).toJson(QJsonDocument::Compact); }
}
ContentResult<core::ContentPage> parseModrinthSearch(const QByteArray& bytes) {
    if (bytes.size() > MaxBody) return ContentError::InvalidResponse;
    const auto doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) return ContentError::InvalidResponse;
    const auto root = doc.object();
    if (!root.value("hits").isArray() || root.value("hits").toArray().size() > 100 ||
        !integer(root.value("offset"), 10000) || !integer(root.value("limit"), 100) || root.value("limit").toInt() < 1 ||
        !integer(root.value("total_hits"), 2147483647)) return ContentError::InvalidResponse;
    core::ContentPage page;
    page.offset = root.value("offset").toInt(); page.limit = root.value("limit").toInt(); page.total = root.value("total_hits").toInt();
    QSet<QString> ids;
    for (const auto& value : root.value("hits").toArray()) {
        if (!value.isObject()) return ContentError::InvalidResponse;
        const auto o = value.toObject();
        if (!text(o,"project_id",8) || !core::validContentId(s(o,"project_id")) || !text(o,"slug",128) ||
            !text(o,"title",256) || !text(o,"description",4096,true) || !integer(o.value("downloads"), 9007199254740991.0) ||
            o.value("project_type").toString() != "mod" || ids.contains(o.value("project_id").toString())) return ContentError::InvalidResponse;
        ids.insert(o.value("project_id").toString());
        page.projects.push_back({core::ContentProvider::Modrinth, s(o,"project_id"), s(o,"slug"), s(o,"title"), s(o,"description"), static_cast<std::uint64_t>(o.value("downloads").toDouble())});
    }
    if (static_cast<int>(page.projects.size()) > page.limit) return ContentError::InvalidResponse;
    return page;
}
ContentResult<std::vector<core::ContentVersion>> parseModrinthVersions(const QByteArray& bytes, const std::string& projectId) {
    if (bytes.size() > MaxBody || !core::validContentId(projectId)) return ContentError::InvalidResponse;
    const auto doc = QJsonDocument::fromJson(bytes);
    if (!doc.isArray() || doc.array().size() > 2000) return ContentError::InvalidResponse;
    std::vector<core::ContentVersion> versions;
    QSet<QString> ids;
    for (const auto& value : doc.array()) {
        if (!value.isObject()) return ContentError::InvalidResponse;
        const auto o = value.toObject();
        core::ContentVersion v;
        if (!text(o,"id",8) || !core::validContentId(s(o,"id")) || s(o,"project_id") != projectId || ids.contains(o.value("id").toString()) ||
            !text(o,"name",512) || !text(o,"version_number",256) || !text(o,"version_type",16) ||
            !strings(o.value("game_versions"),v.gameVersions) || !strings(o.value("loaders"),v.loaders) ||
            !o.value("files").isArray() || o.value("files").toArray().size() > 100 ||
            !o.value("dependencies").isArray() || o.value("dependencies").toArray().size() > 200) return ContentError::InvalidResponse;
        v.id=s(o,"id"); v.projectId=projectId; v.name=s(o,"name"); v.number=s(o,"version_number"); v.releaseType=s(o,"version_type");
        if (v.releaseType!="release" && v.releaseType!="beta" && v.releaseType!="alpha") return ContentError::InvalidResponse;
        ids.insert(o.value("id").toString());
        for (const auto& f : o.value("files").toArray()) {
            if (!f.isObject()) return ContentError::InvalidResponse;
            const auto file=f.toObject();
            if (!text(file,"filename",180) || !text(file,"url",4096) || !integer(file.value("size"),512.0*1024*1024) ||
                !file.value("primary").isBool() || !file.value("hashes").isObject()) return ContentError::InvalidResponse;
            v.files.push_back({s(file,"filename"), {s(file,"url"), s(file.value("hashes").toObject(),"sha1"), static_cast<std::uint64_t>(file.value("size").toDouble())}, file.value("primary").toBool()});
        }
        for (const auto& d : o.value("dependencies").toArray()) {
            if (!d.isObject()) return ContentError::InvalidResponse;
            const auto dep=d.toObject();
            const auto p=s(dep,"project_id"), id=s(dep,"version_id"), type=s(dep,"dependency_type");
            if ((!p.empty() && !core::validContentId(p)) || (!id.empty() && !core::validContentId(id)) ||
                (type!="required" && type!="optional" && type!="incompatible" && type!="embedded")) return ContentError::InvalidResponse;
            v.dependencies.push_back({p,id,type});
        }
        versions.push_back(std::move(v));
    }
    return versions;
}
ModrinthContentSource::ModrinthContentSource(Options options, QObject* parent) : QObject(parent), options_(std::move(options)), network_(std::make_unique<QNetworkAccessManager>()) {}
ModrinthContentSource::~ModrinthContentSource() {
    for (const auto& [id,reply] : replies_) { Q_UNUSED(id); QObject::disconnect(reply,nullptr,this,nullptr); }
    network_.reset();
    replies_.clear();
}
void ModrinthContentSource::get(const QString& id, const QUrl& url, std::function<void(ContentResult<QByteArray>)> done) {
    const bool baseAllowed = options_.base == QUrl("https://api.modrinth.com/v2/") ||
        (options_.allowLoopbackHttp && options_.base.scheme()=="http" && options_.base.host()=="127.0.0.1" && options_.base.userInfo().isEmpty() && !options_.base.hasQuery() && !options_.base.hasFragment());
    if (!baseAllowed || options_.timeoutMs<=0 || id.isEmpty() || replies_.contains(id)) { done(ContentError::InvalidRequest); return; }
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "HustMCL/0.1 (https://github.com/qiu26258-beep/HustMCL)");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto* reply=network_->get(request); replies_[id]=reply;
    auto body=std::make_shared<QByteArray>();
    auto error=std::make_shared<ContentError>(ContentError::None);
    auto* timer=new QTimer(reply); timer->setSingleShot(true);
    connect(timer,&QTimer::timeout,reply,[reply,error] { *error=ContentError::Timeout; reply->abort(); });
    connect(reply,&QNetworkReply::readyRead,reply,[reply,body,error] {
        body->append(reply->readAll()); if (body->size()>MaxBody) { *error=ContentError::InvalidResponse; reply->abort(); }
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply,id,body,error,timer,done=std::move(done)]() mutable {
        timer->stop(); body->append(reply->readAll()); replies_.erase(id);
        const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->property("cancelled").toBool()) *error=ContentError::Cancelled;
        else if (body->size()>MaxBody) *error=ContentError::InvalidResponse;
        else if (*error==ContentError::None) {
            if (status==429) *error=ContentError::RateLimited;
            else if (status==404) *error=ContentError::NotFound;
            else if (status>=300 && status<400) *error=ContentError::InvalidResponse;
            else if (status!=200 || reply->error()!=QNetworkReply::NoError) *error=ContentError::Network;
        }
        reply->deleteLater();
        if (*error!=ContentError::None) done(*error); else done(std::move(*body));
    });
    timer->start(options_.timeoutMs);
}
void ModrinthContentSource::search(const QString& id,const core::ContentQuery& q,std::function<void(ContentResult<core::ContentPage>)> done) {
    if (!core::validContentQuery(q)) { done(ContentError::InvalidRequest); return; }
    auto url=options_.base.resolved(QUrl("search"));
    QJsonArray facets{QJsonArray{QString("project_type:mod")}};
    if (!q.loader.empty()) facets.append(QJsonArray{QString::fromStdString("categories:"+q.loader)});
    if (!q.gameVersion.empty()) facets.append(QJsonArray{QString::fromStdString("versions:"+q.gameVersion)});
    QUrlQuery query; query.addQueryItem("query",QString::fromStdString(q.text)); query.addQueryItem("limit",QString::number(q.limit));
    query.addQueryItem("offset",QString::number(q.offset)); query.addQueryItem("index","relevance");
    query.addQueryItem("facets",QString::fromUtf8(QJsonDocument(facets).toJson(QJsonDocument::Compact))); url.setQuery(query);
    get(id,url,[q,done=std::move(done)](ContentResult<QByteArray> result) mutable {
        if (const auto* e=std::get_if<ContentError>(&result)) { done(*e); return; }
        auto page=parseModrinthSearch(std::get<QByteArray>(result));
        if (const auto* p=std::get_if<core::ContentPage>(&page); p && (p->offset!=q.offset || p->limit!=q.limit)) { done(ContentError::InvalidResponse); return; }
        done(std::move(page));
    });
}
void ModrinthContentSource::versions(const QString& id,const std::string& projectId,const core::ContentQuery& q,std::function<void(ContentResult<std::vector<core::ContentVersion>>)> done) {
    if (!core::validContentId(projectId) || !core::validContentQuery(q)) { done(ContentError::InvalidRequest); return; }
    auto url=options_.base.resolved(QUrl(QString::fromStdString("project/"+projectId+"/version")));
    QUrlQuery query; query.addQueryItem("include_changelog","false");
    if (!q.loader.empty()) query.addQueryItem("loaders",QString::fromUtf8(array(q.loader)));
    if (!q.gameVersion.empty()) query.addQueryItem("game_versions",QString::fromUtf8(array(q.gameVersion)));
    url.setQuery(query);
    get(id,url,[projectId,done=std::move(done)](ContentResult<QByteArray> result) mutable {
        if (const auto* e=std::get_if<ContentError>(&result)) done(*e); else done(parseModrinthVersions(std::get<QByteArray>(result),projectId));
    });
}
void ModrinthContentSource::cancel(const QString& id) {
    const auto it=replies_.find(id); if(it!=replies_.end()) { it->second->setProperty("cancelled",true); it->second->abort(); }
}
} // namespace hustmcl::services
