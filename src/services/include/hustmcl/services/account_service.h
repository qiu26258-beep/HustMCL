#pragma once
#include <hustmcl/application/account.h>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTcpServer>
#include <QTimer>
#include <QUrl>
#include <QJsonObject>
#include <memory>

namespace hustmcl::services {
// A credential file belongs to one launcher data root and one Windows user.
class DpapiCredentialStore final : public application::CredentialStore {
public:
    explicit DpapiCredentialStore(QString path) : path_(std::move(path)) {}
    std::variant<QString, application::AuthFailure> load() override;
    bool save(const QString&) override;
    bool clear() override;
private:
    QString path_;
};
class QtAccountService final : public QObject, public application::AccountService {
    Q_OBJECT
public:
    static QString clientId();
    explicit QtAccountService(QObject* parent = nullptr);
    // Injectable transport for offline protocol tests; destinations remain fixed.
    QtAccountService(QNetworkAccessManager& transport, QObject* parent = nullptr);
    ~QtAccountService() override;
    void signIn(Completion) override;
    void refresh(const QString&, Completion) override;
    void cancel() override;
    void browserFailed();
signals:
    void authorizationRequired(const QUrl& url);
private:
    using JsonDone = std::function<void(QJsonObject)>;
    void initialize();
    void request(const QUrl&, const QByteArray&, const QByteArray& contentType, const QString& bearer, JsonDone);
    void microsoft(const QByteArray& form);
    void xbox(const QString& token);
    void xsts(const QString& token);
    void minecraft(const QString& token);
    void ownership();
    void profile();
    void acceptConnection();
    void exchangeCode();
    void finish(application::AuthResult);
    void fail(application::AuthError, int status = 0);
    std::unique_ptr<QNetworkAccessManager> ownedTransport_;
    QNetworkAccessManager* transport_ = nullptr;
    QPointer<QNetworkReply> reply_;
    QTcpServer callback_;
    QTimer deadline_;
    Completion completion_;
    quint64 generation_ = 0;
    QString state_, verifier_, redirect_, refreshToken_, userHash_;
    QString callbackCode_;
    bool callbackAccepted_ = false;
    application::MinecraftSession session_;
};
}
