#include <hustmcl/services/asset_index.h>
#include <hustmcl/services/version_metadata.h>

#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace hustmcl::core;
using namespace hustmcl::services;
namespace {
void require(bool condition, std::string_view scenario)
{
    if (!condition) { throw std::runtime_error(std::string(scenario)); }
}
QByteArray fixture(const char* name)
{
    QFile file(QString::fromUtf8(HUSTMCL_FIXTURE_DIR) + '/' + QLatin1String(name));
    require(file.open(QIODevice::ReadOnly), "open fixed metadata fixture"); return file.readAll();
}
AssetIndexResult parsePinned(const QByteArray& bytes)
{
    const DownloadDescriptor pin{"https://example.org/index", QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex().toStdString(),
                                 static_cast<std::uint64_t>(bytes.size())};
    return parseAssetIndex(bytes, pin);
}
void expectError(const QByteArray& bytes, PlanErrorCode code = PlanErrorCode::InvalidMetadata)
{
    const auto result = parsePinned(bytes);
    require(std::holds_alternative<PlanError>(result) && std::get<PlanError>(result).code == code, "invalid index rejected with stable error");
    require(std::get<PlanError>(result).field.find("private") == std::string::npos, "error never echoes resource name or raw input");
}
void runTests()
{
    const auto v = std::get<VersionMetadata>(parseVersionMetadata(fixture("1.21.1.json")));
    const auto raw = fixture("17.json");
    require(raw.size() == 449557 && QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex()
            == "e61ea955de95de1f99d36616a8b46147e9f6f5d7", "unaltered official asset index bytes");
    const auto parsed = parseAssetIndex(raw, v.assetIndex);
    require(std::holds_alternative<AssetIndex>(parsed), "actual pinned index accepted");
    const auto plan = std::get<InstallFilePlan>(buildInstallFilePlan(v, std::get<AssetIndex>(parsed)));
    const auto expected = QJsonDocument::fromJson(fixture("expected-install-plan.json")).object();
    require(plan.files.size() == static_cast<std::size_t>(expected["fileCount"].toInt())
            && plan.logicalAssetCount == static_cast<std::size_t>(expected["logicalAssetCount"].toInt())
            && plan.uniqueAssetCount == static_cast<std::size_t>(expected["uniqueAssetCount"].toInt())
            && plan.totalBytes == static_cast<std::uint64_t>(expected["totalBytes"].toDouble()), "independently calculated complete plan totals");
    QByteArray canonical;
    for (const auto& f : plan.files) {
        canonical += QByteArray::number(static_cast<int>(f.role)) + '\t' + QByteArray::fromStdString(f.relativePath.generic_string())
            + '\t' + QByteArray::fromStdString(f.download.sha1) + '\t' + QByteArray::number(static_cast<qulonglong>(f.download.size))
            + '\t' + QByteArray::fromStdString(f.download.url) + '\n';
    }
    require(QCryptographicHash::hash(canonical, QCryptographicHash::Sha256).toHex() == expected["canonicalSha256"].toString().toLatin1(),
            "all 3961 ordered destinations, roles, sizes, hashes and URLs match independent golden digest");
    auto pin = v.assetIndex; pin.sha1 = std::string(40, '0');
    require(std::get<PlanError>(parseAssetIndex(raw, pin)).field == "assetIndex.integrity", "SHA-1 mismatch stops parsing");
    pin = v.assetIndex; ++pin.size;
    require(std::get<PlanError>(parseAssetIndex(raw, pin)).field == "assetIndex.integrity", "size mismatch stops parsing");

    const QByteArray entry("{\"hash\":\"" + QByteArray(40, 'a') + "\",\"size\":0}");
    require(std::holds_alternative<AssetIndex>(parsePinned("{\"objects\":{\"empty\":" + entry + "},\"virtual\":false,\"map_to_resources\":false}")),
            "zero-size object and explicit disabled legacy flags accepted");
    require(std::holds_alternative<AssetIndex>(parsePinned("{\"objects\":{}}")), "empty modern index accepted");
    for (const auto& bytes : {QByteArray("[]"), QByteArray("{"), QByteArray("{}"), QByteArray("{\"objects\":[]}"),
            QByteArray("{\"objects\":{\"private\":1}}"), QByteArray("{\"objects\":{\"private\":{}}}"),
            QByteArray("{\"objects\":{},\"objects\":{}}"), QByteArray("{\"objects\":{},\"obje\\u0063ts\":{}}"),
            QByteArray("{\"objects\":{},\"virtual\":0}"), QByteArray("{\"objects\":{}} trailing")}) {
        expectError(bytes);
    }
    for (const auto* flag : {"virtual", "map_to_resources"}) {
        expectError("{\"objects\":{},\"" + QByteArray(flag) + "\":true}", PlanErrorCode::UnsupportedMetadata);
    }
    expectError("{\"objects\":{},\"future\":false}", PlanErrorCode::UnsupportedMetadata);
    expectError("{\"objects\":{\"private\":{\"hash\":\"" + QByteArray(40, 'a') + "\",\"size\":0,\"future\":false}}}", PlanErrorCode::UnsupportedMetadata);
    for (const auto* size : {"-1", "1.5", "1.0000000000000001", "1e3", "9007199254740992", "18446744073709551615", "true", "\"0\"", "null"}) {
        expectError("{\"objects\":{\"private\":{\"hash\":\"" + QByteArray(40, 'a') + "\",\"size\":" + size + "}}}");
    }
    expectError("{\"objects\":{\"private\":{\"hash\":\"" + QByteArray(40, 'a') + "\",\"size\":0,\"s\\u0069ze\":0}}}");
    expectError("{\"objects\":{\"private\":" + entry + ",\"private\":" + entry + "}}");
    for (const auto& hash : {QByteArray(39, 'a'), QByteArray(40, 'A'), QByteArray("../private"), QByteArray(40, 'g')}) {
        expectError("{\"objects\":{\"private\":{\"hash\":\"" + hash + "\",\"size\":1}}}");
    }
    expectError("{\"objects\":{\"../private\":" + entry + "}}");
    expectError("{\"objects\":{\"\\ud800\":" + entry + "}}");
    expectError("{\"objects\":{\"" + QByteArray(1, static_cast<char>(0xff)) + "\":" + entry + "}}");
    expectError(QByteArray(static_cast<qsizetype>(MaximumAssetIndexBytes + 1), ' '));
}
} // namespace
int main()
{
    try { runTests(); std::cout << "PASS asset index integrity, strict parsing and full golden plan\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
