#pragma once
#include <hustmcl/core/content.h>
#include <QObject>
#include <QString>
#include <functional>
#include <variant>

namespace hustmcl::application {
enum class ContentError { None, InvalidRequest, Network, Timeout, InvalidResponse, RateLimited,
    NotFound, Storage, Integrity, Cancelled, Conflict };
template<class T> using ContentResult = std::variant<T, ContentError>;
class ContentSource {
public:
    virtual ~ContentSource() = default;
    // All completions run on the owning Qt event thread. Cancellation must settle callbacks.
    virtual void search(const QString& operationId, const core::ContentQuery&,
                        std::function<void(ContentResult<core::ContentPage>)>) = 0;
    virtual void versions(const QString& operationId, const std::string& projectId,
                          const core::ContentQuery&, std::function<void(ContentResult<std::vector<core::ContentVersion>>)>) = 0;
    virtual void cancel(const QString& operationId) = 0;
};
struct ContentDownloadRequest {
    std::string projectId, versionId;
    core::ContentFile file;
};
struct ContentDownloadResult { QString path; bool reused = false; };
class ContentDownloadStore {
public:
    virtual ~ContentDownloadStore() = default;
    // Download + verify + publish a named file in an isolated download directory, never mods/saves.
    virtual void acquire(const QString& operationId, const ContentDownloadRequest&,
                         std::function<void(ContentResult<ContentDownloadResult>)>) = 0;
    virtual void cancel(const QString& operationId) = 0;
};
enum class ContentPhase { Idle, Searching, LoadingVersions, Ready, Downloading, Cancelling, Downloaded, Cancelled, Failed };
struct ContentSnapshot {
    QString taskId;
    ContentPhase phase = ContentPhase::Idle;
    core::ContentQuery query;
    core::ContentPage page;
    std::optional<core::ContentProject> selectedProject;
    std::vector<core::ContentVersion> versions;
    QString downloadPath;
    ContentError error = ContentError::None;
    bool reused = false;
};
class ContentController final : public QObject {
    Q_OBJECT
public:
    ContentController(ContentSource&, ContentDownloadStore&, QObject* parent = nullptr);
    ~ContentController() override;
    ContentSnapshot snapshot() const { return state_; }
    bool search(const core::ContentQuery&);
    bool selectProject(const QString& projectId);
    bool download(const QString& versionId);
    bool cancel(const QString& taskId);
signals:
    void changed(const hustmcl::application::ContentSnapshot&);
private:
    bool fileBusy() const;
    QString begin(ContentPhase);
    void fail(ContentError);
    ContentSource& source_;
    ContentDownloadStore& files_;
    ContentSnapshot state_;
};
} // namespace hustmcl::application
Q_DECLARE_METATYPE(hustmcl::application::ContentSnapshot)
