#include <hustmcl/services/account_service.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dpapi.h>

namespace hustmcl::services {
using namespace application;
namespace {
bool safePath(const QString& path) {
    for (QFileInfo info(path); !info.filePath().isEmpty();) {
        if (info.isSymLink() || info.isJunction()) return false;
        const auto parent = info.dir().absolutePath();
        if (parent == info.absoluteFilePath()) break;
        info.setFile(parent);
    }
    return true;
}
}
std::variant<QString, AuthFailure> DpapiCredentialStore::load() {
    QFile file(path_);
    if (!safePath(path_)) return AuthFailure{AuthError::Storage};
    if (!file.exists()) return QString{};
    if (!file.open(QIODevice::ReadOnly) || file.size() > 65536) return AuthFailure{AuthError::Storage};
    auto cipher = file.readAll();
    DATA_BLOB input{static_cast<DWORD>(cipher.size()), reinterpret_cast<BYTE*>(cipher.data())}, output{};
    if (cipher.isEmpty() || !CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &output)) return AuthFailure{AuthError::Storage};
    const auto token = QString::fromUtf8(reinterpret_cast<const char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData); LocalFree(output.pbData);
    if (token.isEmpty() || token.size() > 32768 || token.contains(QChar::Null)) return AuthFailure{AuthError::Storage};
    return token;
}
bool DpapiCredentialStore::save(const QString& token) {
    if (token.isEmpty() || token.size() > 32768 || !safePath(path_)
        || !QDir().mkpath(QFileInfo(path_).absolutePath()) || !safePath(path_)) return false;
    auto plain = token.toUtf8();
    DATA_BLOB input{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE*>(plain.data())}, output{};
    const bool protectedOk = CryptProtectData(&input, L"HustMCL Microsoft refresh credential", nullptr,
        nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
    SecureZeroMemory(plain.data(), static_cast<SIZE_T>(plain.size()));
    if (!protectedOk) return false;
    QSaveFile file(path_);
    const bool saved = file.open(QIODevice::WriteOnly)
        && file.write(reinterpret_cast<const char*>(output.pbData), output.cbData) == output.cbData && file.commit();
    SecureZeroMemory(output.pbData, output.cbData); LocalFree(output.pbData);
    return saved;
}
bool DpapiCredentialStore::clear() {
    return safePath(path_) && (!QFileInfo::exists(path_) || QFile::remove(path_));
}
}
