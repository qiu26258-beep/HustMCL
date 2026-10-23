#include <hustmcl/services/install_plan_input.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("hustmcl_install_plan_probe");
    QCommandLineParser parser;
    parser.setApplicationDescription("Read-only game file-plan probe; never downloads or starts Minecraft.");
    parser.addHelpOption();
    parser.addOptions({
        {{"m", "metadata"}, "Absolute path to pinned version metadata.", "path"},
        {{"s", "sha1"}, "Expected version metadata SHA-1 from the trusted source.", "digest"},
        {{"a", "asset-index"}, "Absolute path to the referenced asset index.", "path"},
        {{"o", "output"}, "Optional report path (parent directory must exist).", "path"}
    });
    parser.process(app);
    for (const auto* option : {"metadata", "sha1", "asset-index"}) {
        if (!parser.isSet(QLatin1String(option))) { parser.showHelp(2); }
    }
    const auto result = hustmcl::services::loadInstallFilePlan(parser.value("metadata"), parser.value("sha1").toLatin1(), parser.value("asset-index"));
    QJsonObject report{{"downloaded", false}, {"installationVerified", false}, {"realGameStarted", false}};
    int exitCode = 0;
    if (const auto error = std::get_if<hustmcl::application::PreparationError>(&result)) {
        report.insert("status", "failed");
        report.insert("errorCode", static_cast<int>(error->code));
        if (error->planError) {
            report.insert("planError", static_cast<int>(error->planError->code));
            report.insert("field", QString::fromStdString(error->planError->field));
        }
        exitCode = 1;
    } else {
        const auto& plan = std::get<hustmcl::core::InstallFilePlan>(result);
        QByteArray canonical;
        for (const auto& f : plan.files) {
            canonical += QByteArray::number(static_cast<int>(f.role)) + '\t' + QByteArray::fromStdString(f.relativePath.generic_string())
                + '\t' + QByteArray::fromStdString(f.download.sha1) + '\t' + QByteArray::number(static_cast<qulonglong>(f.download.size))
                + '\t' + QByteArray::fromStdString(f.download.url) + '\n';
        }
        report.insert("status", "planned");
        report.insert("version", QString::fromStdString(plan.versionId));
        report.insert("javaMajorRequired", plan.java.majorVersion);
        report.insert("fileCount", static_cast<int>(plan.files.size()));
        report.insert("logicalAssetCount", static_cast<int>(plan.logicalAssetCount));
        report.insert("uniqueAssetCount", static_cast<int>(plan.uniqueAssetCount));
        report.insert("totalBytes", QString::number(static_cast<qulonglong>(plan.totalBytes)));
        report.insert("canonicalSha256", QString::fromLatin1(QCryptographicHash::hash(canonical, QCryptographicHash::Sha256).toHex()));
    }
    const auto bytes = QJsonDocument(report).toJson();
    if (parser.isSet("output")) {
        QSaveFile output(parser.value("output"));
        if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) {
            std::cerr << "Cannot write report\n"; exitCode = 3;
        }
    }
    std::cout << bytes.constData();
    return exitCode;
}
