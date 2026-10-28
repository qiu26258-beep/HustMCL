#include <hustmcl/application/account.h>
#include <QPointer>
#include <QRegularExpression>

namespace hustmcl::application {
bool MinecraftSession::valid() const {
    static const QRegularExpression name(QStringLiteral("\\A[A-Za-z0-9_]{1,16}\\z"));
    static const QRegularExpression id(QStringLiteral("\\A[a-fA-F0-9]{32}\\z"));
    for (const auto ch : accessToken) if (ch.unicode() < 33 || ch.unicode() > 126) return false;
    return name.match(playerName).hasMatch() && id.match(uuid).hasMatch()
        && !accessToken.isEmpty() && !clientId.isEmpty()
        && expiresAt > QDateTime::currentDateTimeUtc().addSecs(60);
}
QString authErrorMessage(AuthError error) {
    switch (error) {
    case AuthError::Network: return QStringLiteral("连接登录服务失败，请检查网络后重试。");
    case AuthError::Denied: return QStringLiteral("登录授权已取消，可以重新登录。");
    case AuthError::InvalidRegistration: return QStringLiteral("此应用尚未获得 Minecraft 服务许可。");
    case AuthError::NoOwnership: return QStringLiteral("此账号没有可确认的 Minecraft Java 版授权。");
    case AuthError::NoProfile: return QStringLiteral("未找到 Java 版玩家资料，请先完成账号和玩家名设置。");
    case AuthError::Expired: return QStringLiteral("登录已过期，请重新登录微软账号。");
    case AuthError::Storage: return QStringLiteral("无法读取或保存登录凭据，请检查数据目录权限。");
    case AuthError::Cancelled: return QStringLiteral("已取消登录。");
    case AuthError::Timeout: return QStringLiteral("登录等待超时，请重新打开登录页。");
    case AuthError::Browser: return QStringLiteral("无法打开浏览器，请检查默认浏览器设置后重试。");
    case AuthError::Busy: return QStringLiteral("请等待当前操作完成。");
    case AuthError::InvalidResponse: return QStringLiteral("登录服务返回了无效结果，请稍后重试。");
    }
    return QStringLiteral("登录失败，请重试。");
}
AccountController::AccountController(AccountService& service, CredentialStore& store, QObject* parent)
    : QObject(parent), service_(service), store_(store) {
    state_.providerLabel = QStringLiteral("Microsoft · Minecraft Java");
    state_.message = QStringLiteral("登录微软账号以验证游戏所有权。");
}
AccountController::~AccountController() { ++generation_; service_.cancel(); }
void AccountController::publish() {
    state_.busy = busy_;
    state_.canSignIn = !busy_ && !launchActive_;
    state_.canSignOut = !launchActive_ && (busy_ || !refreshToken_.isEmpty() || !session_.accessToken.isEmpty() || state_.remembered);
    emit changed(state_);
}
bool AccountController::signIn() {
    if (busy_ || launchActive_) return false;
    begin(false); return true;
}
bool AccountController::refresh() {
    if (busy_ || launchActive_ || refreshToken_.isEmpty()) return false;
    begin(true); return true;
}
void AccountController::restore() {
    if (busy_ || launchActive_ || !session_.accessToken.isEmpty() || !refreshToken_.isEmpty()) return;
    const auto stored = store_.load();
    if (std::holds_alternative<AuthFailure>(stored)) {
        state_.state = AccountState::Error; state_.message = authErrorMessage(AuthError::Storage);
        publish(); return;
    }
    refreshToken_ = std::get<QString>(stored);
    state_.remembered = !refreshToken_.isEmpty();
    if (!refreshToken_.isEmpty()) begin(true); else publish();
}
void AccountController::begin(bool refreshing) {
    const auto generation = ++generation_;
    busy_ = true;
    session_ = {};
    state_.state = AccountState::SigningIn;
    state_.message = refreshing ? QStringLiteral("正在恢复账号并验证游戏所有权…") : QStringLiteral("请在系统浏览器中完成微软登录。");
    const QPointer<AccountController> guard(this);
    publish();
    if (!guard || generation != generation_ || !busy_) return;
    auto done = [guard, generation](AuthResult result) {
        if (guard) guard->completed(generation, std::move(result));
    };
    if (refreshing) service_.refresh(refreshToken_, std::move(done));
    else service_.signIn(std::move(done));
}
void AccountController::completed(quint64 generation, AuthResult result) {
    if (generation != generation_ || !busy_) return;
    busy_ = false;
    if (const auto failure = std::get_if<AuthFailure>(&result)) {
        session_ = {};
        state_.state = failure->code == AuthError::Expired ? AccountState::Expired : AccountState::Error;
        state_.message = authErrorMessage(failure->code);
        publish(); return;
    }
    auto success = std::get<AuthSuccess>(std::move(result));
    if (!success.session.valid() || success.refreshToken.isEmpty()) {
        state_.state = AccountState::Error; state_.message = authErrorMessage(AuthError::InvalidResponse);
        publish(); return;
    }
    session_ = std::move(success.session);
    if (!success.refreshToken.isEmpty()) refreshToken_ = std::move(success.refreshToken);
    state_.remembered = !refreshToken_.isEmpty() && store_.save(refreshToken_);
    state_.state = AccountState::SignedIn;
    state_.displayName = session_.playerName;
    state_.message = state_.remembered ? QStringLiteral("已验证 Java 版所有权，登录已安全保存。")
        : QStringLiteral("已验证 Java 版所有权；登录未能保存，重开后需重新登录。");
    publish();
}
bool AccountController::signOut() {
    if (launchActive_) return false;
    ++generation_; busy_ = false; service_.cancel(); session_ = {}; refreshToken_.clear();
    const bool cleared = store_.clear();
    state_.state = cleared ? AccountState::SignedOut : AccountState::Error;
    state_.displayName.clear(); state_.remembered = !cleared;
    state_.message = cleared ? QStringLiteral("已退出 HustMCL；浏览器中的微软登录状态由浏览器管理。")
        : QStringLiteral("当前会话已退出，但未能删除保存的凭据，请检查数据目录权限。");
    publish(); return cleared;
}
void AccountController::cancel() {
    if (!busy_) return;
    ++generation_; busy_ = false; service_.cancel(); session_ = {};
    state_.state = AccountState::SignedOut; state_.message = authErrorMessage(AuthError::Cancelled);
    publish();
}
void AccountController::setLaunchActive(bool active) { launchActive_ = active; publish(); }
}
