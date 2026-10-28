#include <hustmcl/services/account_service.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrlQuery>
#include <QTcpSocket>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace hustmcl;
using namespace application;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Response { int status; QByteArray bytes; int delayMs = 0; };
class Reply final : public QNetworkReply {
public:
    Reply(const QNetworkRequest& request, Response response, QObject* parent) : QNetworkReply(parent), bytes_(std::move(response.bytes)) {
        setRequest(request); setUrl(request.url()); setAttribute(QNetworkRequest::HttpStatusCodeAttribute, response.status);
        if (response.status >= 400) setError(QNetworkReply::ContentAccessDenied, "sanitized test error");
        open(QIODevice::ReadOnly);
        QTimer::singleShot(response.delayMs, this, [this] { emit readyRead(); setFinished(true); emit finished(); });
    }
    void abort() override {}
    qint64 bytesAvailable() const override { return bytes_.size() - pos_ + QIODevice::bytesAvailable(); }
protected:
    qint64 readData(char* data, qint64 length) override {
        const auto count = qMin(length, bytes_.size() - pos_);
        if (!count) return -1;
        std::memcpy(data, bytes_.constData() + pos_, static_cast<size_t>(count)); pos_ += count; return count;
    }
private:
    QByteArray bytes_; qint64 pos_ = 0;
};
class Transport final : public QNetworkAccessManager {
public:
    std::vector<Response> responses;
    std::vector<QNetworkRequest> requests;
    std::vector<QByteArray> bodies;
protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice* data) override {
        requests.push_back(request); bodies.push_back(data ? data->readAll() : QByteArray{});
        require(requests.size() <= responses.size(), "no calls after terminal failure");
        return new Reply(request, responses[requests.size() - 1], this);
    }
};
Response json(QJsonObject value) { return {200, QJsonDocument(value).toJson(QJsonDocument::Compact)}; }
std::vector<Response> happy() {
    const QJsonObject claims{{"xui", QJsonArray{QJsonObject{{"uhs", "123"}, {"xid", "456"}}}}};
    return {json({{"access_token", "fake-ms"}, {"refresh_token", "fake-rotated"}}),
        json({{"Token", "fake-xbox"}, {"DisplayClaims", claims}}),
        json({{"Token", "fake-xsts"}, {"DisplayClaims", claims}}),
        json({{"access_token", "fake-minecraft"}, {"expires_in", 3600}}),
        json({{"items", QJsonArray{QJsonObject{{"name", "game_minecraft"}}}}}),
        json({{"id", QString(32, 'a')}, {"name", "Player_One"}})};
}
AuthResult refresh(Transport& transport, const QString& credential = QStringLiteral("fake-refresh")) {
    services::QtAccountService service(transport); QEventLoop loop;
    AuthResult result = AuthFailure{AuthError::Timeout};
    service.refresh(credential, [&](AuthResult value) { result = std::move(value); loop.quit(); });
    QTimer::singleShot(2000, &loop, &QEventLoop::quit); loop.exec(); return result;
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        for (int stage = 0; stage < 6; ++stage) {
            Transport transport; transport.responses = happy(); transport.responses[stage] = {403, "{}"};
            const auto result = refresh(transport);
            require(std::holds_alternative<AuthFailure>(result) && transport.requests.size() == static_cast<size_t>(stage + 1), "each failed auth stage stops later requests");
        }
        Transport transport; transport.responses = happy();
        const auto good = refresh(transport);
        require(std::holds_alternative<AuthSuccess>(good), "full protocol chain succeeds");
        require(std::get<AuthSuccess>(good).refreshToken == "fake-rotated" && std::get<AuthSuccess>(good).session.xuid == "456", "rotated credential and xuid retained");
        require(transport.requests[0].url().host() == "login.microsoftonline.com"
            && transport.bodies[0].contains("grant_type=refresh_token") && !transport.bodies[0].contains("client_secret"), "public client refresh request");
        require(transport.requests[5].rawHeader("Authorization") == "Bearer fake-minecraft", "profile uses minecraft session");
        Transport special; special.responses = happy();
        require(std::holds_alternative<AuthSuccess>(refresh(special, "fake+a/b=c&d"))
            && special.bodies[0].contains("refresh_token=fake%2Ba%2Fb%3Dc%26d"), "OAuth form encodes reserved credential characters without changing their value");
        for (const auto& request : transport.requests) require(request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt() == QNetworkRequest::ManualRedirectPolicy, "credential requests refuse redirects");
        Transport noOwner; noOwner.responses = happy(); noOwner.responses[4] = json({{"items", QJsonArray{}}});
        require(std::get<AuthFailure>(refresh(noOwner)).code == AuthError::NoOwnership && noOwner.requests.size() == 5, "ownership required before player profile");
        Transport mismatch; mismatch.responses = happy(); mismatch.responses[2] = json({{"Token", "fake"}, {"DisplayClaims", QJsonObject{{"xui", QJsonArray{QJsonObject{{"uhs", "999"}}}}}}});
        require(std::get<AuthFailure>(refresh(mismatch)).code == AuthError::InvalidResponse && mismatch.requests.size() == 3, "xsts must match same xbox user");
        Transport expired; expired.responses = {{400, "{\"error\":\"invalid_grant\",\"error_description\":\"private\"}"}};
        require(std::get<AuthFailure>(refresh(expired)).code == AuthError::Expired, "refresh revocation is actionable");
        Transport redirected; redirected.responses = {{302, "{}"}};
        require(std::get<AuthFailure>(refresh(redirected)).code == AuthError::InvalidResponse, "redirect is failure");
        Transport large; large.responses = {{200, QByteArray(1024 * 1024 + 1, 'x')}};
        require(std::get<AuthFailure>(refresh(large)).code == AuthError::InvalidResponse, "oversized response bounded");
        {
            Transport fake; fake.responses = happy(); services::QtAccountService service(fake);
            fake.responses[0].delayMs = 80;
            QUrl authorization;
            QObject::connect(&service, &services::QtAccountService::authorizationRequired, &app, [&](const QUrl& url) { authorization = url; });
            bool completed = false;
            service.signIn([&](AuthResult result) { completed = std::holds_alternative<AuthSuccess>(result); });
            QUrlQuery query(authorization);
            require(query.queryItemValue("code_challenge_method") == "S256"
                && query.queryItemValue("scope") == "XboxLive.signin offline_access"
                && query.queryItemValue("state").size() >= 43 && !query.hasQueryItem("code_verifier"), "PKCE authorization URL exposes only challenge");
            QNetworkAccessManager browser;
            auto callback = QUrl(query.queryItemValue("redirect_uri"));
            callback.setQuery("state=wrong&code=fake-code");
            auto* invalid = browser.get(QNetworkRequest(callback));
            QEventLoop invalidLoop; QObject::connect(invalid, &QNetworkReply::finished, &invalidLoop, &QEventLoop::quit); invalidLoop.exec();
            require(invalid->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 400 && !completed, "wrong state rejected without ending valid wait");
            invalid->deleteLater();
            QTcpSocket late;
            const QUrl redirect(query.queryItemValue("redirect_uri"));
            late.connectToHost(QHostAddress::LocalHost, static_cast<quint16>(redirect.port()));
            require(late.waitForConnected(1000), "pending local connection established before callback completion");
            QCoreApplication::processEvents();
            QUrlQuery valid; valid.addQueryItem("state", query.queryItemValue("state")); valid.addQueryItem("code", "fake-code"); callback.setQuery(valid);
            auto* accepted = browser.get(QNetworkRequest(callback));
            QObject::connect(accepted, &QNetworkReply::finished, &app, [&] {
                late.write("GET /?state=&code=fake-late HTTP/1.1\r\nHost: " + redirect.authority().toLatin1() + "\r\nConnection: close\r\n\r\n");
            });
            QEventLoop fullLoop;
            QTimer poll; poll.setInterval(1); QObject::connect(&poll, &QTimer::timeout, &fullLoop, [&] { if (completed) fullLoop.quit(); }); poll.start();
            QTimer::singleShot(3000, &fullLoop, &QEventLoop::quit); fullLoop.exec();
            require(completed && accepted->url().path() == "/complete" && !accepted->url().hasQuery(), "real callback redirects to clean page and exchanges code");
            require(fake.bodies[0].contains("code_verifier=") && fake.bodies[0].contains("grant_type=authorization_code"), "PKCE verifier used only on token exchange");
            require(fake.requests.size() == 6 && late.readAll().startsWith("HTTP/1.1 400"), "pending connections cannot submit another code after PKCE state has been consumed");
        }
        QTemporaryDir temporary; require(temporary.isValid(), "credential temporary root");
        const auto path = temporary.filePath("credentials/microsoft.dpapi");
        services::DpapiCredentialStore store(path);
        require(std::get<QString>(store.load()).isEmpty(), "missing credentials are signed out");
        require(store.save("fake-private-refresh") && std::get<QString>(store.load()) == "fake-private-refresh", "DPAPI roundtrip for current Windows user");
        QFile encrypted(path); require(encrypted.open(QIODevice::ReadOnly), "cipher open");
        require(!encrypted.readAll().contains("fake-private-refresh"), "credential never stored as plaintext"); encrypted.close();
        require(encrypted.open(QIODevice::WriteOnly | QIODevice::Truncate), "corrupt test credential"); encrypted.write("corrupt"); encrypted.close();
        require(std::holds_alternative<AuthFailure>(store.load()), "corrupt credential reported without silent overwrite");
        require(store.clear() && !QFile::exists(path), "logout removes only owned credential");
        std::cout << "account protocol, callback and DPAPI scenarios passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
