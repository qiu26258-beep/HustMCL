#include <hustmcl/services/launch_input_source.h>
#include <hustmcl/services/version_metadata.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QPointer>
#include <QTimer>

#include <utility>

namespace hustmcl::services {
using namespace application;

MetadataFileSource::MetadataFileSource(QString instanceId, QString file, QByteArray expectedSha1,
                                     LaunchInput settings, QObject* parent)
    : QObject(parent), instanceId_(std::move(instanceId)), file_(std::move(file)),
      expectedSha1_(std::move(expectedSha1)), settings_(std::move(settings))
{
    workers_.setMaxThreadCount(1);
}

MetadataFileSource::~MetadataFileSource()
{
    // Keep this QObject alive until the bounded read has posted its result.
    // Queued results are discarded by QObject destruction, never invoked on a dead receiver.
    workers_.waitForDone();
}

void MetadataFileSource::load(const QString& instanceId, Completion complete)
{
    if (instanceId != instanceId_ || busy_) {
        QTimer::singleShot(0, this, [complete] { complete(PreparationError{PreparationErrorCode::InputUnavailable, {}, {}}); });
        return;
    }
    busy_ = true;
    const QPointer<MetadataFileSource> guard(this);
    const auto path = file_;
    const auto expected = expectedSha1_;
    const auto settings = settings_;
    workers_.start([guard, path, expected, settings, complete = std::move(complete)]() mutable {
        const auto read = [&]() -> LaunchInputResult {
            if (!QDir::isAbsolutePath(path) || path.startsWith(QLatin1String("\\\\")) || path.startsWith(QLatin1String("//"))) {
                return PreparationError{PreparationErrorCode::InputUnavailable, {}, {}};
            }
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly) || file.size() < 1 || file.size() > 4 * 1024 * 1024) {
                return PreparationError{PreparationErrorCode::InputUnavailable, {}, {}};
            }
            const auto bytes = file.read(4 * 1024 * 1024 + 1);
            if (!file.atEnd() || file.error() != QFileDevice::NoError || bytes.size() > 4 * 1024 * 1024
                || expected.size() != 40 || QCryptographicHash::hash(bytes, QCryptographicHash::Sha1).toHex() != expected) {
                return PreparationError{PreparationErrorCode::InvalidInput, {}, {}};
            }
            auto parsed = parseVersionMetadata(bytes);
            if (const auto error = std::get_if<core::PlanError>(&parsed)) {
                return PreparationError{PreparationErrorCode::InvalidInput, {}, *error};
            }
            auto result = settings;
            result.version = std::get<core::VersionMetadata>(std::move(parsed));
            return result;
        };
        auto result = read();
        if (guard) {
            QMetaObject::invokeMethod(guard.data(), [guard, complete = std::move(complete), result = std::move(result)]() mutable {
                if (guard) {
                    guard->busy_ = false;
                    complete(std::move(result));
                }
            }, Qt::QueuedConnection);
        }
    });
}

} // namespace hustmcl::services
