#include <hustmcl/application/offline_launch.h>
#include <QCryptographicHash>
#include <QUuid>

namespace hustmcl::application {
OfflineLaunchResult makeOfflineLaunch(const core::LaunchPlan& plan, const QString& playerName) {
    if (playerName.isEmpty() || playerName.size() > 16) return OfflineLaunchError::InvalidPlayerName;
    for (const auto ch : playerName) {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_'))
            return OfflineLaunchError::InvalidPlayerName;
    }
    auto digest = QCryptographicHash::hash("OfflinePlayer:" + playerName.toUtf8(), QCryptographicHash::Md5);
    digest[6] = static_cast<char>((static_cast<unsigned char>(digest[6]) & 0x0f) | 0x30);
    digest[8] = static_cast<char>((static_cast<unsigned char>(digest[8]) & 0x3f) | 0x80);
    OfflineLaunch result{playerName, QUuid::fromRfc4122(digest).toString(QUuid::WithoutBraces), {}};
    for (const auto& argument : plan.arguments) {
        QString value;
        for (const auto& part : argument) {
            if (const auto literal = std::get_if<std::string>(&part)) value += QString::fromStdString(*literal);
            else switch (std::get<core::SessionField>(part)) {
                case core::SessionField::PlayerName: value += playerName; break;
                case core::SessionField::Uuid: { auto compact = result.uuid; compact.remove('-'); value += compact; break; }
                case core::SessionField::AccessToken:
                case core::SessionField::ClientId:
                case core::SessionField::Xuid: value += '0'; break;
            }
        }
        if (value == "--demo") return OfflineLaunchError::DemoPlan;
        result.arguments.append(value);
    }
    return result;
}
}
