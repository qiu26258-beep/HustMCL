#pragma once
#include <hustmcl/application/frontend_snapshot.h>
#include <QDateTime>
#include <QObject>
#include <functional>
#include <variant>

namespace hustmcl::application {
enum class AuthError { Network, Denied, InvalidResponse, InvalidRegistration, NoOwnership,
    NoProfile, Expired, Storage, Cancelled, Timeout, Browser, Busy };
struct AuthFailure { AuthError code; int httpStatus = 0; };
// Backend-only values. Never put these in a page snapshot, diagnostic or plan.
struct MinecraftSession {
    QString playerName, uuid, accessToken, clientId, xuid;
    QDateTime expiresAt;
    bool valid() const;
};
struct AuthSuccess { MinecraftSession session; QString refreshToken; };
using AuthResult = std::variant<AuthSuccess, AuthFailure>;
class AccountService {
public:
    using Completion = std::function<void(AuthResult)>;
    virtual ~AccountService() = default;
    virtual void signIn(Completion) = 0;
    virtual void refresh(const QString& refreshToken, Completion) = 0;
    virtual void cancel() = 0;
};
class CredentialStore {
public:
    virtual ~CredentialStore() = default;
    // Empty string means no saved account. Corruption is an explicit failure.
    virtual std::variant<QString, AuthFailure> load() = 0;
    virtual bool save(const QString& refreshToken) = 0;
    virtual bool clear() = 0;
};
class AccountController final : public QObject {
    Q_OBJECT
public:
    AccountController(AccountService&, CredentialStore&, QObject* parent = nullptr);
    ~AccountController() override;
    AccountSnapshot snapshot() const { return state_; }
    const MinecraftSession* session() const { return session_.valid() ? &session_ : nullptr; }
    bool signIn();
    bool refresh();
    void restore();
    bool signOut();
    void cancel();
    void setLaunchActive(bool active);
signals:
    void changed(const hustmcl::application::AccountSnapshot& snapshot);
private:
    void begin(bool refreshing);
    void completed(quint64 generation, AuthResult);
    void publish();
    AccountService& service_;
    CredentialStore& store_;
    AccountSnapshot state_;
    MinecraftSession session_;
    QString refreshToken_;
    quint64 generation_ = 0;
    bool busy_ = false, launchActive_ = false;
};
QString authErrorMessage(AuthError);
}
