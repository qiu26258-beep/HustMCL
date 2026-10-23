#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <iostream>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QJsonArray args;
    for (const auto& arg : app.arguments().mid(1)) args.append(arg);
    const auto result = QJsonDocument(QJsonObject{{"arguments", args},
        {"javaOptions", qEnvironmentVariable("JAVA_TOOL_OPTIONS")},
        {"classpath", qEnvironmentVariable("CLASSPATH")}}).toJson(QJsonDocument::Compact);
    std::cout << result.constData() << std::flush;
    QThread::msleep(400);
    return app.arguments().contains("--fail") ? 7 : 0;
}
