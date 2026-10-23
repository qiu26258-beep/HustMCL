#include <hustmcl/services/local_game.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cstdlib>
#include <iostream>
using namespace hustmcl;
void check(bool ok, const char* message) { if (!ok) { std::cerr << message << '\n'; std::exit(1); } }
void write(const QString& path, const QByteArray& bytes) {
    check(QDir().mkpath(QFileInfo(path).absolutePath()), "create fixture directory");
    QFile f(path); check(f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size(), "write fixture");
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    const QDir root(temporary.path());
    const QByteArray payload("x");
    const auto sha = QCryptographicHash::hash(payload, QCryptographicHash::Sha1).toHex();
    const QJsonObject descriptor{{"url", "https://example.org/x"}, {"sha1", QString::fromLatin1(sha)}, {"size", 1}};
    QFile fixture(QString::fromUtf8(HUSTMCL_FIXTURE_DIR) + "/1.21.1.json");
    check(fixture.open(QIODevice::ReadOnly), "read metadata fixture");
    auto version = QJsonDocument::fromJson(fixture.readAll()).object();
    version.insert("id", "local-test");
    version.insert("downloads", QJsonObject{{"client", descriptor}});
    auto artifact = descriptor; artifact.insert("path", "test/base.jar");
    version.insert("libraries", QJsonArray{QJsonObject{{"name", "test:base:1"}, {"downloads", QJsonObject{{"artifact", artifact}}}}});
    auto loggingFile = descriptor; loggingFile.insert("id", "local.xml");
    auto loggingClient = version.value("logging").toObject().value("client").toObject();
    loggingClient.insert("file", loggingFile);
    version.insert("logging", QJsonObject{{"client", loggingClient}});
    const auto index = QJsonDocument(QJsonObject{{"objects", QJsonObject{{"x", QJsonObject{{"hash", QString::fromLatin1(sha)}, {"size", 1}}}}}}).toJson(QJsonDocument::Compact);
    auto indexDescriptor = descriptor;
    indexDescriptor.insert("id", "local-index");
    indexDescriptor.insert("sha1", QString::fromLatin1(QCryptographicHash::hash(index, QCryptographicHash::Sha1).toHex()));
    indexDescriptor.insert("size", static_cast<int>(index.size()));
    version.insert("assetIndex", indexDescriptor);
    version.insert("assets", "local-index");
    write(root.filePath("versions/local-test/local-test.json"), QJsonDocument(version).toJson());
    write(root.filePath("assets/indexes/local-index.json"), index);
    for (const auto& relative : QStringList{"libraries/test/base.jar", "versions/local-test/local-test.jar", "assets/log_configs/local.xml",
            "assets/objects/" + QString::fromLatin1(sha.left(2)) + '/' + QString::fromLatin1(sha)}) write(root.filePath(relative), payload);
    auto result = services::verifyLocalGame(root.path(), "local-test");
    check(std::holds_alternative<services::LocalGame>(result) && std::get<services::LocalGame>(result).files.files.size() == 5, "complete installation verified");
    write(root.filePath("libraries/test/base.jar"), "y");
    check(std::get<services::LocalGameError>(services::verifyLocalGame(root.path(), "local-test")) == services::LocalGameError::MissingOrCorruptFile,
        "same-size corrupted library blocks launch");
    check(QFile::remove(root.filePath("libraries/test/base.jar")), "remove owned fixture");
    check(std::get<services::LocalGameError>(services::verifyLocalGame(root.path(), "local-test")) == services::LocalGameError::MissingOrCorruptFile,
        "missing library blocks launch");
    check(std::get<services::LocalGameError>(services::verifyLocalGame(root.path(), "../escape")) == services::LocalGameError::UnsafePath, "version escape rejected");
    write(root.filePath("game"), "preserve");
    check(!services::prepareLocalGameDirectories(root.path()) && QFileInfo(root.filePath("game")).isFile(), "obstruction preserved");
    std::cout << "Local installation integrity tests passed\n";
}
