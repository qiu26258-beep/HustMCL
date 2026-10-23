#include <hustmcl/services/file_acquirer.h>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <iostream>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 5) { std::cerr << "usage: hustmcl_file_probe DATA_ROOT HTTPS_URL SHA1 SIZE\n"; return 2; }
    bool valid = false;
    const auto size = args[4].toULongLong(&valid);
    if (!valid) return 2;
    hustmcl::services::QtFileAcquirer service({});
    QTimer::singleShot(0, &app, [&] {
        service.acquire("probe", args[1], {args[2].toStdString(), args[3].toStdString(), size}, [&](auto result) {
            QJsonObject report{{"error", static_cast<int>(result.error)}, {"cachePath", result.cachePath},
                {"reused", result.reused}, {"installationVerified", false}, {"realGameStarted", false}};
            std::cout << QJsonDocument(report).toJson().toStdString();
            app.exit(result.error == hustmcl::application::FileError::None ? 0 : 1);
        });
    });
    return app.exec();
}
