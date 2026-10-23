#include <hustmcl/services/asset_index.h>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QStringConverter>

#include <cmath>
#include <vector>

namespace hustmcl::services {
namespace {
using namespace core;
PlanError invalid(const char* field) { return {PlanErrorCode::InvalidMetadata, field}; }

bool whitespace(char ch) { return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t'; }

// Qt deliberately keeps the last duplicate member. After Qt syntax validation,
// examine original member tokens so escaped spellings cannot hide duplicates.
std::optional<PlanError> checkTokens(const QByteArray& bytes)
{
    std::vector<QSet<QString>> members;
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        const auto ch = bytes[i];
        if (ch == '{') { members.emplace_back(); }
        else if (ch == '}') { members.pop_back(); }
        else if (ch == '"') {
            const auto start = i++;
            while (i < bytes.size() && bytes[i] != '"') {
                if (bytes[i] == '\\') { ++i; }
                ++i;
            }
            auto next = i + 1;
            while (next < bytes.size() && whitespace(bytes[next])) { ++next; }
            if (next < bytes.size() && bytes[next] == ':') {
                const auto decoded = QJsonDocument::fromJson('[' + bytes.mid(start, i - start + 1) + ']').array()[0].toString();
                if (members.back().contains(decoded)) { return invalid("assetIndex.json.duplicateKey"); }
                members.back().insert(decoded);
            }
        } else if (ch == '-' || (ch >= '0' && ch <= '9')) {
            const auto start = i;
            while (i + 1 < bytes.size() && !whitespace(bytes[i + 1]) && bytes[i + 1] != ','
                   && bytes[i + 1] != '}' && bytes[i + 1] != ']') { ++i; }
            // Asset sizes use canonical nonnegative decimal integers. Reject even
            // sub-double-precision fractions rather than accepting rounded values.
            for (auto j = start; j <= i; ++j) {
                if (bytes[j] < '0' || bytes[j] > '9') { return invalid("assetIndex.objects.size"); }
            }
        }
    }
    return {};
}
} // namespace

AssetIndexResult parseAssetIndex(const QByteArray& json, const core::DownloadDescriptor& expected)
{
    if (json.isEmpty() || json.size() > static_cast<qsizetype>(MaximumAssetIndexBytes)
        || static_cast<std::uint64_t>(json.size()) != expected.size
        || QCryptographicHash::hash(json, QCryptographicHash::Sha1).toHex() != QByteArray::fromStdString(expected.sha1)) {
        return invalid("assetIndex.integrity");
    }
    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    const QString decodedText = decoder.decode(json);
    if (decodedText.isEmpty() || decoder.hasError()) { return invalid("assetIndex.json.encoding"); }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) { return invalid("assetIndex.json"); }
    if (auto error = checkTokens(json)) { return *error; }
    const auto root = document.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.key() == QLatin1String("virtual") || it.key() == QLatin1String("map_to_resources")) {
            if (!it.value().isBool()) { return invalid("assetIndex.legacyFlags"); }
            if (it.value().toBool()) { return PlanError{PlanErrorCode::UnsupportedMetadata, "assetIndex.legacyFlags"}; }
        } else if (it.key() != QLatin1String("objects")) {
            return PlanError{PlanErrorCode::UnsupportedMetadata, "assetIndex.fields"};
        }
    }
    if (!root.value("objects").isObject()) { return invalid("assetIndex.objects"); }
    const auto objects = root.value("objects").toObject();
    if (objects.size() > static_cast<qsizetype>(MaximumAssetObjects)) { return invalid("assetIndex.objects.limit"); }
    AssetIndex index{expected.sha1, expected.size, {}};
    index.objects.reserve(static_cast<std::size_t>(objects.size()));
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (!it.value().isObject()) { return invalid("assetIndex.objects[]"); }
        const auto entry = it.value().toObject();
        for (auto field = entry.begin(); field != entry.end(); ++field) {
            if (field.key() != QLatin1String("hash") && field.key() != QLatin1String("size")) {
                return PlanError{PlanErrorCode::UnsupportedMetadata, "assetIndex.objects.fields"};
            }
        }
        if (!entry.value("hash").isString() || !entry.value("size").isDouble()) { return invalid("assetIndex.objects[]"); }
        const auto size = entry.value("size").toDouble();
        if (!std::isfinite(size) || size < 0 || size > 9007199254740991.0 || std::floor(size) != size) {
            return invalid("assetIndex.objects.size");
        }
        index.objects.push_back({it.key().toStdString(), entry.value("hash").toString().toStdString(), static_cast<std::uint64_t>(size)});
    }
    if (auto error = validateAssetIndex(index)) { return *error; }
    return index;
}
} // namespace hustmcl::services
