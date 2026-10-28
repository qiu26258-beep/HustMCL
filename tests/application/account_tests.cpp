#include <hustmcl/application/account.h>
#include <hustmcl/application/authenticated_launch.h>
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace hustmcl::application;
namespace core = hustmcl::core;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Service : AccountService {
    std::vector<Completion> calls;
    QString refreshed;
    void signIn(Completion done) override { calls.push_back(std::move(done)); }
    void refresh(const QString& token, Completion done) override { refreshed = token; calls.push_back(std::move(done)); }
    void cancel() override {}
};
struct Store : CredentialStore {
    QString token;
    bool fail = false;
    int writes = 0;
    std::variant<QString, AuthFailure> load() override { return token; }
    bool save(const QString& value) override { ++writes; if (fail) return false; token = value; return true; }
    bool clear() override { if (fail) return false; token.clear(); return true; }
};
AuthSuccess success() {
    return {{"Player_One", QString(32, 'a'), "private-minecraft", "client-id", "123", QDateTime::currentDateTimeUtc().addSecs(3600)}, "private-refresh"};
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        Service service; Store store; AccountController account(service, store);
        require(!account.session(), "no initial authenticated session");
        require(account.signIn() && !account.signIn(), "duplicate login rejected");
        require(account.snapshot().state == AccountState::SigningIn, "visible login state");
        account.signOut(); service.calls[0](success());
        require(!account.session() && store.writes == 0, "logout rejects delayed login before credential save");
        account.signIn(); account.cancel(); service.calls[1](success());
        require(!account.session() && store.writes == 0, "cancel rejects delayed login");
        account.signIn(); service.calls[2](AuthFailure{AuthError::NoOwnership});
        require(!account.session() && store.writes == 0 && account.snapshot().state == AccountState::Error, "no ownership cannot become authenticated");
        account.signIn(); service.calls[3](success());
        require(account.session() && store.token == "private-refresh" && account.snapshot().remembered, "validated account saved behind credential port");
        require(!account.snapshot().message.contains("private") && account.snapshot().displayName == "Player_One", "snapshot contains no credentials");
        account.setLaunchActive(true);
        require(!account.signOut() && !account.signIn() && !account.refresh(), "identity locked during launch and game");
        account.setLaunchActive(false);
        require(account.refresh(), "refresh starts from existing credential");
        auto rotated = success(); rotated.refreshToken = "private-rotated";
        service.calls[4](rotated);
        require(service.refreshed == "private-refresh" && store.token == "private-rotated", "refresh credential rotation persists");
        require(account.signOut() && store.token.isEmpty() && !account.session(), "logout removes owned credential and memory session");
        store.token = "restore-token";
        account.restore(); require(service.refreshed == "restore-token", "restart restores using refresh flow");
        service.calls[5](AuthFailure{AuthError::Expired});
        require(!account.session() && account.snapshot().state == AccountState::Expired, "revoked credentials cannot launch");
        account.signOut(); store.fail = true;
        account.signIn(); service.calls[6](success());
        require(account.session() && !account.snapshot().remembered, "save failure remains visible without pretending persistence");
        require(!account.signOut() && !account.session() && account.snapshot().state == AccountState::Error, "logout storage failure reported with memory cleared");
        auto expired = success(); expired.session.expiresAt = QDateTime::currentDateTimeUtc().addSecs(-1);
        account.signIn(); service.calls[7](expired);
        require(!account.session(), "expired service results never accepted");
        core::LaunchPlan plan;
        plan.arguments = {{std::string("--username")}, {core::SessionField::PlayerName},
            {core::SessionField::Uuid}, {core::SessionField::AccessToken}, {core::SessionField::ClientId}, {core::SessionField::Xuid}};
        const auto launch = std::get<GameLaunch>(makeAuthenticatedLaunch(plan, success().session));
        require(launch.authenticated && launch.arguments == QStringList{"--username", "Player_One", QString(32, 'a'), "private-minecraft", "client-id", "123"}, "typed launch fields use validated online identity");
        require(launch.redactions == QStringList{"private-minecraft"}, "token supplied to output redaction only behind backend boundary");
        require(std::holds_alternative<AuthFailure>(makeAuthenticatedLaunch(plan, expired.session)), "expired identity cannot form authenticated launch");
        plan.arguments.push_back({std::string("--demo")});
        require(std::holds_alternative<AuthFailure>(makeAuthenticatedLaunch(plan, success().session)), "authenticated entitlement cannot become a demo launch");
        auto malformed = success().session; malformed.playerName += '\n';
        require(!malformed.valid(), "profile name rejects trailing newline");
        malformed = success().session; malformed.accessToken += '\n';
        require(!malformed.valid(), "session token rejects control characters");
        account.signIn(); auto noCredential = success(); noCredential.refreshToken.clear();
        service.calls[8](noCredential);
        require(!account.session() && account.snapshot().state == AccountState::Error, "new profile without its own renewal credential cannot reuse a previous account credential");
        std::cout << "account lifecycle and delayed callback scenarios passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
