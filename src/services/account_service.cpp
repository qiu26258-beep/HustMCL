#include <hustmcl/services/account_service.h>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QSharedPointer>

namespace hustmcl::services {
using namespace application;
namespace {
const QUrl authorize(QStringLiteral("https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"));
const QUrl tokenEndpoint(QStringLiteral("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
const QUrl userEndpoint(QStringLiteral("https://user.auth.xboxlive.com/user/authenticate"));
const QUrl xstsEndpoint(QStringLiteral("https://xsts.auth.xboxlive.com/xsts/authorize"));
const QUrl minecraftEndpoint(QStringLiteral("https://api.minecraftservices.com/authentication/login_with_xbox"));
const QUrl ownershipEndpoint(QStringLiteral("https://api.minecraftservices.com/entitlements/mcstore"));
const QUrl profileEndpoint(QStringLiteral("https://api.minecraftservices.com/minecraft/profile"));
QString randomValue() {
    QByteArray bytes(48, Qt::Uninitialized);
    for (qsizetype i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<char>(QRandomGenerator::system()->generate());
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}
QByteArray form(const QList<QPair<QString, QString>>& values) {
    QByteArray body;
    for (const auto& entry : values) {
        if (!body.isEmpty()) body += '&';
        body += QUrl::toPercentEncoding(entry.first) + '=' + QUrl::toPercentEncoding(entry.second);
    }
    return body;
}
bool safeToken(const QString& value) {
    if (value.isEmpty() || value.size() > 65536) return false;
    for (const auto ch : value) if (ch.unicode() < 33 || ch.unicode() > 126) return false;
    return true;
}
QJsonObject claim(const QJsonObject& data) {
    const auto claims = data.value("DisplayClaims").toObject().value("xui").toArray();
    return claims.isEmpty() ? QJsonObject{} : claims.first().toObject();
}
}
QString QtAccountService::clientId() { return QStringLiteral("b208e1c1-3ebb-4be3-ba08-2d8f8b19d71c"); }
QtAccountService::QtAccountService(QObject* parent) : QObject(parent), ownedTransport_(std::make_unique<QNetworkAccessManager>()), transport_(ownedTransport_.get()) { initialize(); }
QtAccountService::QtAccountService(QNetworkAccessManager& transport, QObject* parent) : QObject(parent), transport_(&transport) { initialize(); }
void QtAccountService::initialize() {
    deadline_.setSingleShot(true);
    connect(&deadline_, &QTimer::timeout, this, [this] { fail(AuthError::Timeout); });
    connect(&callback_, &QTcpServer::newConnection, this, &QtAccountService::acceptConnection);
}
QtAccountService::~QtAccountService() { cancel(); }
void QtAccountService::cancel() {
    ++generation_; completion_ = {}; deadline_.stop(); callback_.close();
    auto active = reply_; reply_.clear();
    if (active) { active->abort(); active->deleteLater(); }
    state_.clear(); verifier_.clear(); redirect_.clear(); callbackCode_.clear();
    callbackAccepted_ = false;
    refreshToken_.clear(); userHash_.clear(); session_ = {};
}
void QtAccountService::finish(AuthResult result) {
    auto done = std::move(completion_);
    cancel();
    if (done) done(std::move(result));
}
void QtAccountService::fail(AuthError error, int status) { finish(AuthFailure{error, status}); }
void QtAccountService::browserFailed() { if (completion_) fail(AuthError::Browser); }
void QtAccountService::signIn(Completion done) {
    cancel(); completion_ = std::move(done);
    if (!callback_.listen(QHostAddress::LocalHost, 0)) { fail(AuthError::Browser); return; }
    state_ = randomValue(); verifier_ = randomValue();
    redirect_ = QStringLiteral("http://localhost:%1").arg(callback_.serverPort());
    const auto challenge = QCryptographicHash::hash(verifier_.toLatin1(), QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QUrl url(authorize);
    QUrlQuery query;
    for (const auto& item : QList<QPair<QString, QString>>{{"client_id", clientId()}, {"response_type", "code"},
        {"redirect_uri", redirect_}, {"scope", "XboxLive.signin offline_access"}, {"response_mode", "query"},
        {"code_challenge_method", "S256"}, {"code_challenge", QString::fromLatin1(challenge)},
        {"state", state_}, {"prompt", "select_account"}}) query.addQueryItem(item.first, item.second);
    url.setQuery(query); deadline_.start(600000);
    emit authorizationRequired(url);
}
void QtAccountService::refresh(const QString& credential, Completion done) {
    cancel(); completion_ = std::move(done); refreshToken_ = credential;
    if (!safeToken(credential)) { fail(AuthError::Expired); return; }
    deadline_.start(180000);
    microsoft(form({{"client_id", clientId()}, {"grant_type", "refresh_token"},
        {"refresh_token", credential}, {"scope", "XboxLive.signin offline_access"}}));
}
void QtAccountService::exchangeCode() {
    if (!completion_ || callbackCode_.isEmpty()) return;
    const auto code = std::move(callbackCode_); callbackCode_.clear(); callback_.close();
    deadline_.start(180000);
    microsoft(form({{"client_id", clientId()}, {"grant_type", "authorization_code"},
        {"code", code}, {"redirect_uri", redirect_}, {"code_verifier", verifier_}}));
    verifier_.clear(); state_.clear();
}
void QtAccountService::acceptConnection() {
    while (callback_.hasPendingConnections()) {
        auto* socket = callback_.nextPendingConnection();
        const auto generation = generation_;
        auto buffer = QSharedPointer<QByteArray>::create();
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        QTimer::singleShot(10000, socket, [socket] { socket->disconnectFromHost(); });
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer, generation] {
            buffer->append(socket->readAll());
            if (generation != generation_ || buffer->size() > 16384) { socket->disconnectFromHost(); return; }
            if (!buffer->contains("\r\n\r\n")) return;
            const auto lines = buffer->split('\n');
            const auto first = lines.value(0).trimmed().split(' ');
            QByteArray host; int hosts = 0;
            for (const auto& line : lines) if (line.left(5).toLower() == "host:") { host = line.mid(5).trimmed(); ++hosts; }
            auto respond = [socket](const QByteArray& status, const QByteArray& extra, const QByteArray& body) {
                socket->write("HTTP/1.1 " + status + "\r\nCache-Control: no-store\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: default-src 'none'\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n" + extra + "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            };
            if (first.size() != 3 || first[0] != "GET" || hosts != 1 || host != QUrl(redirect_).authority().toLatin1()) {
                respond("400 Bad Request", {}, {}); return;
            }
            if (first[1] == "/complete" && !callbackCode_.isEmpty()) {
                respond("200 OK", {}, "<!doctype html><title>HustMCL</title>HustMCL received the sign-in response. Return to the launcher.");
                exchangeCode(); return;
            }
            const auto target = QUrl::fromEncoded(first[1]);
            const QUrlQuery query(target);
            const auto items = query.queryItems(QUrl::FullyDecoded);
            auto unique = [&](const QString& key) {
                int count = 0; for (const auto& item : items) if (item.first == key) ++count;
                return count;
            };
            if (!target.isRelative() || !target.authority().isEmpty() || target.path() != "/" || target.hasFragment()
                || callbackAccepted_ || state_.isEmpty() || verifier_.isEmpty()
                || unique("state") != 1 || query.queryItemValue("state", QUrl::FullyDecoded) != state_
                || items.size() > 12 || unique("code") > 1 || unique("error") > 1
                || (unique("code") + unique("error")) != 1) { respond("400 Bad Request", {}, {}); return; }
            if (unique("error")) { respond("200 OK", {}, "Login cancelled. Return to HustMCL."); fail(AuthError::Denied); return; }
            callbackCode_ = query.queryItemValue("code", QUrl::FullyDecoded);
            if (!safeToken(callbackCode_)) { callbackCode_.clear(); respond("400 Bad Request", {}, {}); return; }
            callbackAccepted_ = true;
            respond("303 See Other", "Location: /complete\r\n", {});
            QTimer::singleShot(2000, this, [this, generation] { if (generation == generation_) exchangeCode(); });
        });
    }
}
void QtAccountService::request(const QUrl& url, const QByteArray& body, const QByteArray& contentType,
    const QString& bearer, JsonDone done) {
    if (!completion_) return;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(25000);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "HustMCL/0.1.0");
    if (!contentType.isEmpty()) request.setRawHeader("Content-Type", contentType);
    if (!bearer.isEmpty()) request.setRawHeader("Authorization", "Bearer " + bearer.toUtf8());
    if (url == userEndpoint || url == xstsEndpoint) request.setRawHeader("x-xbl-contract-version", "1");
    auto* reply = body.isEmpty() ? transport_->get(request) : transport_->post(request, body);
    reply_ = reply;
    const auto generation = generation_;
    auto bytes = QSharedPointer<QByteArray>::create();
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, bytes, generation] {
        if (generation != generation_) return;
        bytes->append(reply->readAll());
        if (bytes->size() > 1024 * 1024) fail(AuthError::InvalidResponse);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, generation, url, done = std::move(done)] {
        reply->deleteLater();
        if (generation != generation_ || !completion_) return;
        reply_.clear(); bytes->append(reply->readAll());
        if (bytes->size() > 1024 * 1024) { fail(AuthError::InvalidResponse); return; }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QJsonParseError parse;
        const auto document = QJsonDocument::fromJson(*bytes, &parse);
        const auto object = document.object();
        if (status < 200 || status >= 300 || reply->error() != QNetworkReply::NoError) {
            AuthError error = AuthError::Network;
            if (url == tokenEndpoint && object.value("error") == "invalid_grant") error = AuthError::Expired;
            else if (url == minecraftEndpoint && status == 403 &&
                (object.value("errorMessage").toString().contains("Invalid app registration", Qt::CaseInsensitive)
                 || object.value("message").toString().contains("Invalid app registration", Qt::CaseInsensitive))) error = AuthError::InvalidRegistration;
            else if (status == 401) error = AuthError::Expired;
            else if (url == profileEndpoint && status == 404) error = AuthError::NoProfile;
            else if (status >= 300 && status < 400) error = AuthError::InvalidResponse;
            fail(error, status); return;
        }
        if (parse.error != QJsonParseError::NoError || !document.isObject()) { fail(AuthError::InvalidResponse); return; }
        done(object);
    });
}
void QtAccountService::microsoft(const QByteArray& body) {
    request(tokenEndpoint, body, "application/x-www-form-urlencoded", {}, [this](const QJsonObject& value) {
        const auto token = value.value("access_token").toString();
        const auto refresh = value.value("refresh_token").toString();
        if (!safeToken(token) || (!refresh.isEmpty() && !safeToken(refresh))
            || (refresh.isEmpty() && refreshToken_.isEmpty())) { fail(AuthError::InvalidResponse); return; }
        if (!refresh.isEmpty()) refreshToken_ = refresh;
        xbox(token);
    });
}
void QtAccountService::xbox(const QString& token) {
    const QJsonObject body{{"RelyingParty", "http://auth.xboxlive.com"}, {"TokenType", "JWT"},
        {"Properties", QJsonObject{{"AuthMethod", "RPS"}, {"SiteName", "user.auth.xboxlive.com"}, {"RpsTicket", "d=" + token}}}};
    request(userEndpoint, QJsonDocument(body).toJson(QJsonDocument::Compact), "application/json", {}, [this](const QJsonObject& value) {
        const auto token = value.value("Token").toString();
        const auto claims = claim(value);
        userHash_ = claims.value("uhs").toString(); session_.xuid = claims.value("xid").toString();
        static const QRegularExpression digits(QStringLiteral("^[0-9]+$"));
        if (!safeToken(token) || !digits.match(userHash_).hasMatch()) { fail(AuthError::InvalidResponse); return; }
        xsts(token);
    });
}
void QtAccountService::xsts(const QString& token) {
    const QJsonObject body{{"RelyingParty", "rp://api.minecraftservices.com/"}, {"TokenType", "JWT"},
        {"Properties", QJsonObject{{"SandboxId", "RETAIL"}, {"UserTokens", QJsonArray{token}}}}};
    request(xstsEndpoint, QJsonDocument(body).toJson(QJsonDocument::Compact), "application/json", {}, [this](const QJsonObject& value) {
        const auto token = value.value("Token").toString();
        const auto claims = claim(value);
        if (!safeToken(token) || claims.value("uhs").toString() != userHash_) { fail(AuthError::InvalidResponse); return; }
        if (!claims.value("xid").toString().isEmpty()) session_.xuid = claims.value("xid").toString();
        minecraft(token);
    });
}
void QtAccountService::minecraft(const QString& token) {
    const QJsonObject body{{"identityToken", "XBL3.0 x=" + userHash_ + ";" + token}};
    request(minecraftEndpoint, QJsonDocument(body).toJson(QJsonDocument::Compact), "application/json", {}, [this](const QJsonObject& value) {
        session_.accessToken = value.value("access_token").toString(); session_.clientId = clientId();
        const double seconds = value.value("expires_in").toDouble();
        if (!safeToken(session_.accessToken) || seconds < 61 || seconds > 86400) { fail(AuthError::InvalidResponse); return; }
        session_.expiresAt = QDateTime::currentDateTimeUtc().addSecs(static_cast<qint64>(seconds)); ownership();
    });
}
void QtAccountService::ownership() {
    request(ownershipEndpoint, {}, {}, session_.accessToken, [this](const QJsonObject& value) {
        const auto items = value.value("items").toArray();
        bool owned = false;
        for (const auto& item : items) {
            const auto name = item.toObject().value("name").toString();
            if (name == "game_minecraft" || name == "product_minecraft") owned = true;
        }
        if (!owned) { fail(AuthError::NoOwnership); return; }
        profile();
    });
}
void QtAccountService::profile() {
    request(profileEndpoint, {}, {}, session_.accessToken, [this](const QJsonObject& value) {
        session_.uuid = value.value("id").toString(); session_.playerName = value.value("name").toString();
        if (!session_.valid()) { fail(AuthError::InvalidResponse); return; }
        finish(AuthSuccess{session_, refreshToken_});
    });
}
}
