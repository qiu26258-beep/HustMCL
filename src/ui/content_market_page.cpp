#include "content_market_page.h"

#include "page_header.h"
#include "primary_action_bar.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLocale>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>

namespace hustmcl::ui {

namespace {

QString textFromStd(const std::string& value)
{
    return QString::fromStdString(value);
}

QString releaseTypeText(const std::string& type)
{
    if (type == "release") {
        return QStringLiteral("正式版");
    }
    if (type == "beta") {
        return QStringLiteral("测试版");
    }
    if (type == "alpha") {
        return QStringLiteral("预览版");
    }
    return textFromStd(type);
}

QString loaderText(const std::string& loader)
{
    if (loader == "fabric") {
        return QStringLiteral("Fabric");
    }
    if (loader == "forge") {
        return QStringLiteral("Forge");
    }
    if (loader == "neoforge") {
        return QStringLiteral("NeoForge");
    }
    if (loader == "quilt") {
        return QStringLiteral("Quilt");
    }
    return textFromStd(loader);
}

} // namespace

ContentMarketPage::ContentMarketPage(application::ContentController* controller, QWidget* parent)
    : PageScaffold(parent)
{
    header()->setContent(QStringLiteral("模组市场"),
                         QStringLiteral("浏览 Modrinth 上的模组与资源"),
                         QStringLiteral("Modrinth"));

    m_content = new QWidget;
    auto* layout = new QVBoxLayout(m_content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(0);

    auto* searchTitle = new QLabel(QStringLiteral("搜索模组"), m_content);
    searchTitle->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(searchTitle);
    layout->addSpacing(4);
    auto* searchNote = new QLabel(
        QStringLiteral("按关键词查找 Modrinth 项目；Minecraft 版本和加载器可以留空。"),
        m_content);
    searchNote->setObjectName(QStringLiteral("DetailNote"));
    searchNote->setWordWrap(true);
    layout->addWidget(searchNote);
    layout->addSpacing(8);

    m_searchText = new QLineEdit(m_content);
    m_searchText->setObjectName(QStringLiteral("MarketSearchInput"));
    m_searchText->setPlaceholderText(QStringLiteral("搜索模组名称或描述"));
    m_searchText->setClearButtonEnabled(true);
    m_searchText->setMaxLength(200);
    m_searchText->setAccessibleName(QStringLiteral("搜索关键词"));
    m_gameVersion = new QLineEdit(m_content);
    m_gameVersion->setObjectName(QStringLiteral("MarketVersionInput"));
    m_gameVersion->setPlaceholderText(QStringLiteral("MC 版本（可选）"));
    m_gameVersion->setClearButtonEnabled(true);
    m_gameVersion->setMaxLength(32);
    m_gameVersion->setFixedWidth(150);
    m_gameVersion->setAccessibleName(QStringLiteral("Minecraft 版本筛选"));
    m_loader = new QComboBox(m_content);
    m_loader->setObjectName(QStringLiteral("MarketLoader"));
    m_loader->setAccessibleName(QStringLiteral("加载器筛选"));
    m_loader->addItem(QStringLiteral("全部加载器"), QString());
    m_loader->addItem(QStringLiteral("Fabric"), QStringLiteral("fabric"));
    m_loader->addItem(QStringLiteral("Forge"), QStringLiteral("forge"));
    m_loader->addItem(QStringLiteral("NeoForge"), QStringLiteral("neoforge"));
    m_loader->addItem(QStringLiteral("Quilt"), QStringLiteral("quilt"));
    m_searchButton = new QPushButton(QStringLiteral("搜索"), m_content);
    m_searchButton->setObjectName(QStringLiteral("MarketSearch"));
    m_searchButton->setCursor(Qt::PointingHandCursor);
    m_searchButton->setAutoDefault(false);
    m_searchButton->setAccessibleName(QStringLiteral("搜索 Modrinth"));

    m_searchGrid = new QGridLayout();
    m_searchGrid->setContentsMargins(0, 0, 0, 0);
    m_searchGrid->setHorizontalSpacing(8);
    m_searchGrid->setVerticalSpacing(8);
    m_searchGrid->addWidget(m_searchText, 0, 0);
    m_searchGrid->addWidget(m_gameVersion, 0, 1);
    m_searchGrid->addWidget(m_loader, 0, 2);
    m_searchGrid->addWidget(m_searchButton, 0, 3);
    m_searchGrid->setColumnStretch(0, 1);
    layout->addLayout(m_searchGrid);
    layout->addSpacing(12);

    auto* divider = new QFrame(m_content);
    divider->setObjectName(QStringLiteral("Divider"));
    divider->setFixedHeight(1);
    layout->addWidget(divider);
    layout->addSpacing(12);

    m_body = new QBoxLayout(QBoxLayout::LeftToRight);
    m_body->setContentsMargins(0, 0, 0, 0);
    m_body->setSpacing(20);

    m_resultsPane = new QWidget(m_content);
    m_resultsPane->setMaximumWidth(330);
    auto* resultsLayout = new QVBoxLayout(m_resultsPane);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    resultsLayout->setSpacing(6);
    auto* resultsTitle = new QLabel(QStringLiteral("搜索结果"), m_resultsPane);
    resultsTitle->setObjectName(QStringLiteral("SectionTitle"));
    resultsLayout->addWidget(resultsTitle);
    m_resultsSummary = new QLabel(m_resultsPane);
    m_resultsSummary->setObjectName(QStringLiteral("DetailNote"));
    m_resultsSummary->setWordWrap(true);
    resultsLayout->addWidget(m_resultsSummary);
    m_results = new QListWidget(m_resultsPane);
    m_results->setObjectName(QStringLiteral("MarketResults"));
    m_results->setAccessibleName(QStringLiteral("模组搜索结果"));
    m_results->setWordWrap(true);
    m_results->setUniformItemSizes(true);
    m_results->setMinimumHeight(220);
    m_results->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsLayout->addWidget(m_results, 1);
    auto* pager = new QHBoxLayout();
    pager->setContentsMargins(0, 0, 0, 0);
    pager->setSpacing(8);
    m_previousPage = new QPushButton(QStringLiteral("上一页"), m_resultsPane);
    m_previousPage->setObjectName(QStringLiteral("DialogAction"));
    m_previousPage->setAutoDefault(false);
    m_nextPage = new QPushButton(QStringLiteral("下一页"), m_resultsPane);
    m_nextPage->setObjectName(QStringLiteral("DialogAction"));
    m_nextPage->setAutoDefault(false);
    m_pageLabel = new QLabel(m_resultsPane);
    m_pageLabel->setObjectName(QStringLiteral("PagerLabel"));
    m_pageLabel->setAlignment(Qt::AlignCenter);
    pager->addWidget(m_previousPage);
    pager->addWidget(m_pageLabel, 1);
    pager->addWidget(m_nextPage);
    resultsLayout->addLayout(pager);

    m_bodyDivider = new QFrame(m_content);
    m_bodyDivider->setObjectName(QStringLiteral("VDivider"));
    m_bodyDivider->setFixedWidth(1);

    m_detailPane = new QWidget(m_content);
    auto* detailLayout = new QVBoxLayout(m_detailPane);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(5);
    m_projectTitle = new QLabel(m_detailPane);
    m_projectTitle->setObjectName(QStringLiteral("MarketProjectTitle"));
    m_projectTitle->setTextFormat(Qt::PlainText);
    m_projectTitle->setWordWrap(true);
    detailLayout->addWidget(m_projectTitle);
    m_projectDescription = new QLabel(m_detailPane);
    m_projectDescription->setObjectName(QStringLiteral("MarketProjectDescription"));
    m_projectDescription->setTextFormat(Qt::PlainText);
    m_projectDescription->setWordWrap(true);
    detailLayout->addWidget(m_projectDescription);
    m_projectMeta = new QLabel(m_detailPane);
    m_projectMeta->setObjectName(QStringLiteral("DetailLabel"));
    m_projectMeta->setTextFormat(Qt::PlainText);
    detailLayout->addWidget(m_projectMeta);
    m_filterLabel = new QLabel(m_detailPane);
    m_filterLabel->setObjectName(QStringLiteral("DetailNote"));
    m_filterLabel->setTextFormat(Qt::PlainText);
    m_filterLabel->setWordWrap(true);
    detailLayout->addWidget(m_filterLabel);
    detailLayout->addSpacing(6);

    m_versionsTitle = new QLabel(m_detailPane);
    m_versionsTitle->setObjectName(QStringLiteral("SectionTitle"));
    m_versionsTitle->setTextFormat(Qt::PlainText);
    detailLayout->addWidget(m_versionsTitle);
    m_versions = new QListWidget(m_detailPane);
    m_versions->setObjectName(QStringLiteral("MarketVersions"));
    m_versions->setAccessibleName(QStringLiteral("模组版本列表"));
    m_versions->setWordWrap(true);
    m_versions->setUniformItemSizes(true);
    m_versions->setMinimumHeight(110);
    m_versions->setSelectionMode(QAbstractItemView::SingleSelection);
    detailLayout->addWidget(m_versions);
    m_dependency = new QLabel(m_detailPane);
    m_dependency->setObjectName(QStringLiteral("MarketDependency"));
    m_dependency->setTextFormat(Qt::PlainText);
    m_dependency->setWordWrap(true);
    detailLayout->addWidget(m_dependency);

    m_status = new QLabel(m_detailPane);
    m_status->setObjectName(QStringLiteral("MarketStatus"));
    m_status->setTextFormat(Qt::PlainText);
    m_status->setWordWrap(true);
    detailLayout->addWidget(m_status);

    m_errorNotice = new QFrame(m_detailPane);
    m_errorNotice->setObjectName(QStringLiteral("ErrorNotice"));
    auto* errorLayout = new QVBoxLayout(m_errorNotice);
    errorLayout->setContentsMargins(12, 10, 12, 10);
    m_errorLabel = new QLabel(m_errorNotice);
    m_errorLabel->setObjectName(QStringLiteral("ErrorTitle"));
    m_errorLabel->setTextFormat(Qt::PlainText);
    m_errorLabel->setWordWrap(true);
    errorLayout->addWidget(m_errorLabel);
    detailLayout->addWidget(m_errorNotice);
    detailLayout->addSpacing(6);

    m_note = new QLabel(
        QStringLiteral("下载只保存文件，不会修改当前游戏。安装前请确认加载器与游戏版本匹配；"
                       "启动器不会自动安装前置或加载器。"),
        m_detailPane);
    m_note->setObjectName(QStringLiteral("DetailNote"));
    m_note->setWordWrap(true);
    detailLayout->addWidget(m_note);
    detailLayout->addStretch(1);

    m_body->addWidget(m_resultsPane, 0);
    m_body->addWidget(m_bodyDivider, 0);
    m_body->addWidget(m_detailPane, 1);
    layout->addLayout(m_body);
    setContent(m_content);

    m_actionBar = actionBar();
    connect(m_searchButton, &QPushButton::clicked, this, [this]() { submitSearch(0); });
    connect(m_searchText, &QLineEdit::returnPressed, this, [this]() { submitSearch(0); });
    connect(m_gameVersion, &QLineEdit::returnPressed, this, [this]() { submitSearch(0); });
    connect(m_results, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!m_controller || !m_results->item(row) || m_snapshot.phase == application::ContentPhase::Downloading
            || m_snapshot.phase == application::ContentPhase::Cancelling) {
            return;
        }
        const QString projectId = m_results->item(row)->data(Qt::UserRole).toString();
        if (projectId.isEmpty()) {
            return;
        }
        m_selectedVersionId.clear();
        m_lastProjectId = projectId;
        m_controller->selectProject(projectId);
    });
    connect(m_versions, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!m_versions->item(row)) {
            if (m_snapshot.phase != application::ContentPhase::Downloading
                && m_snapshot.phase != application::ContentPhase::Cancelling) {
                m_selectedVersionId.clear();
            }
        } else {
            m_selectedVersionId = m_versions->item(row)->data(Qt::UserRole).toString();
        }
        updateDependency();
        updateActionBar();
    });
    connect(m_previousPage, &QPushButton::clicked, this, [this]() {
        if (!m_controller) {
            return;
        }
        core::ContentQuery query = m_snapshot.query;
        const int limit = query.limit > 0 ? query.limit : 20;
        query.offset = std::max(0, query.offset - limit);
        m_controller->search(query);
    });
    connect(m_nextPage, &QPushButton::clicked, this, [this]() {
        if (!m_controller) {
            return;
        }
        core::ContentQuery query = m_snapshot.query;
        const int limit = query.limit > 0 ? query.limit : 20;
        query.offset = query.offset + limit;
        m_controller->search(query);
    });
    connect(m_actionBar, &PrimaryActionBar::primaryClicked, this, [this]() {
        const core::ContentVersion* version = selectedVersion();
        if (m_controller && version) {
            m_controller->download(textFromStd(version->id));
        }
    });
    connect(m_actionBar, &PrimaryActionBar::secondaryClicked,
            this, &ContentMarketPage::handleSecondary);

    setController(controller);
    updateResponsiveLayout();
}

void ContentMarketPage::setController(application::ContentController* controller)
{
    if (m_controller.data() == controller) {
        if (!m_initialized) {
            setSnapshot({});
        }
        return;
    }
    if (m_controller) {
        disconnect(m_controller.data(), &application::ContentController::changed,
                   this, &ContentMarketPage::setSnapshot);
    }
    m_controller = controller;
    m_selectedVersionId.clear();
    m_lastProjectId.clear();
    m_initialized = false;
    if (m_controller) {
        connect(m_controller.data(), &application::ContentController::changed,
                this, &ContentMarketPage::setSnapshot);
        setSnapshot(m_controller->snapshot());
    } else {
        setSnapshot({});
    }
}

application::ContentController* ContentMarketPage::controller() const
{
    return m_controller.data();
}

void ContentMarketPage::setSnapshot(const application::ContentSnapshot& snapshot)
{
    m_snapshot = snapshot;
    if (!m_initialized) {
        m_initialized = true;
        m_searchText->setText(textFromStd(snapshot.query.text));
        m_gameVersion->setText(textFromStd(snapshot.query.gameVersion));
        const QString loader = textFromStd(snapshot.query.loader);
        const int loaderIndex = m_loader->findData(loader);
        m_loader->setCurrentIndex(loaderIndex >= 0 ? loaderIndex : 0);
    }

    const QString projectId = snapshot.selectedProject
        ? textFromStd(snapshot.selectedProject->id)
        : QString();
    if (projectId != m_lastProjectId) {
        m_selectedVersionId.clear();
        m_lastProjectId = projectId;
    }
    if (snapshot.phase == application::ContentPhase::Searching) {
        m_selectedVersionId.clear();
    }

    m_errorNotice->setVisible(snapshot.error != application::ContentError::None);
    if (snapshot.error != application::ContentError::None) {
        m_errorLabel->setText(QStringLiteral("错误码 %1 · %2")
                                  .arg(errorCode(snapshot.error), errorText(snapshot.error)));
    }

    updateResults();
    updateProject();
    updateVersions();
    updateSearchState();
    updateActionBar();
}

void ContentMarketPage::focusPrimaryAction()
{
    if (m_searchText->isEnabled()) {
        m_searchText->setFocus(Qt::OtherFocusReason);
        if (auto* area = scrollArea()) {
            area->ensureWidgetVisible(m_searchText);
        }
        return;
    }
    if (!m_actionBar->primaryButton()->isHidden()
        && m_actionBar->primaryButton()->isEnabled()) {
        m_actionBar->primaryButton()->setFocus(Qt::OtherFocusReason);
    }
}

void ContentMarketPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void ContentMarketPage::updateResponsiveLayout()
{
    if (!m_body) {
        return;
    }
    const bool vertical = width() < 820;
    if (vertical) {
        m_body->setDirection(QBoxLayout::TopToBottom);
        m_body->setSpacing(16);
        m_bodyDivider->hide();
        m_resultsPane->setMaximumWidth(QWIDGETSIZE_MAX);
        m_resultsPane->setMinimumWidth(0);
        m_results->setMinimumHeight(170);
    } else {
        m_body->setDirection(QBoxLayout::LeftToRight);
        m_body->setSpacing(20);
        m_bodyDivider->show();
        m_resultsPane->setMaximumWidth(330);
        m_resultsPane->setMinimumWidth(280);
        m_results->setMinimumHeight(220);
    }
    updateSearchLayout();
}

void ContentMarketPage::updateSearchLayout()
{
    if (!m_searchGrid) {
        return;
    }
    m_searchGrid->removeWidget(m_searchText);
    m_searchGrid->removeWidget(m_gameVersion);
    m_searchGrid->removeWidget(m_loader);
    m_searchGrid->removeWidget(m_searchButton);
    for (int column = 0; column < 4; ++column) {
        m_searchGrid->setColumnStretch(column, 0);
    }
    if (width() < 820) {
        m_searchGrid->addWidget(m_searchText, 0, 0, 1, 4);
        m_searchGrid->addWidget(m_gameVersion, 1, 0);
        m_searchGrid->addWidget(m_loader, 1, 1);
        m_searchGrid->addWidget(m_searchButton, 1, 2);
        m_searchGrid->setColumnStretch(3, 1);
    } else {
        m_searchGrid->addWidget(m_searchText, 0, 0);
        m_searchGrid->addWidget(m_gameVersion, 0, 1);
        m_searchGrid->addWidget(m_loader, 0, 2);
        m_searchGrid->addWidget(m_searchButton, 0, 3);
        m_searchGrid->setColumnStretch(0, 1);
    }
}

void ContentMarketPage::submitSearch(int offset)
{
    if (!m_controller) {
        return;
    }
    core::ContentQuery query;
    query.text = m_searchText->text().trimmed().toStdString();
    query.gameVersion = m_gameVersion->text().trimmed().toStdString();
    query.loader = m_loader->currentData().toString().toStdString();
    query.offset = offset;
    query.limit = 20;
    m_selectedVersionId.clear();
    m_controller->search(query);
}

const core::ContentVersion* ContentMarketPage::selectedVersion() const
{
    for (const core::ContentVersion& version : m_snapshot.versions) {
        if (textFromStd(version.id) == m_selectedVersionId) {
            return &version;
        }
    }
    return nullptr;
}

QString ContentMarketPage::appliedFilterText() const
{
    const QString text = m_snapshot.query.text.empty()
        ? QStringLiteral("全部关键词")
        : QStringLiteral("关键词「%1」").arg(textFromStd(m_snapshot.query.text));
    const QString gameVersion = m_snapshot.query.gameVersion.empty()
        ? QStringLiteral("全部 MC 版本")
        : QStringLiteral("MC %1").arg(textFromStd(m_snapshot.query.gameVersion));
    const QString loader = m_snapshot.query.loader.empty()
        ? QStringLiteral("全部加载器")
        : loaderText(m_snapshot.query.loader);
    return QStringLiteral("当前筛选：%1 · %2 · %3").arg(text, gameVersion, loader);
}

QString ContentMarketPage::errorText(application::ContentError error) const
{
    switch (error) {
    case application::ContentError::None:
        return QStringLiteral("操作已完成。");
    case application::ContentError::InvalidRequest:
        return QStringLiteral("搜索条件无效，请调整后重试。");
    case application::ContentError::Network:
        return QStringLiteral("无法连接 Modrinth，请检查网络后重试。");
    case application::ContentError::Timeout:
        return QStringLiteral("连接 Modrinth 超时，请稍后重试。");
    case application::ContentError::InvalidResponse:
        return QStringLiteral("Modrinth 返回了无法识别的数据，请稍后重试。");
    case application::ContentError::RateLimited:
        return QStringLiteral("请求过于频繁，请稍后再试。");
    case application::ContentError::NotFound:
        return QStringLiteral("没有找到对应的项目或版本。");
    case application::ContentError::Storage:
        return QStringLiteral("无法保存下载文件，请检查下载目录。");
    case application::ContentError::Integrity:
        return QStringLiteral("下载文件校验失败，未保存到下载目录。");
    case application::ContentError::Cancelled:
        return QStringLiteral("操作已取消。");
    case application::ContentError::Conflict:
        return QStringLiteral("下载目录中已有同名但内容不同的文件，请先处理该文件。");
    }
    return QStringLiteral("操作失败，请稍后重试。");
}

QString ContentMarketPage::errorCode(application::ContentError error) const
{
    switch (error) {
    case application::ContentError::None:
        return QStringLiteral("NONE");
    case application::ContentError::InvalidRequest:
        return QStringLiteral("INVALID_REQUEST");
    case application::ContentError::Network:
        return QStringLiteral("NETWORK");
    case application::ContentError::Timeout:
        return QStringLiteral("TIMEOUT");
    case application::ContentError::InvalidResponse:
        return QStringLiteral("INVALID_RESPONSE");
    case application::ContentError::RateLimited:
        return QStringLiteral("RATE_LIMITED");
    case application::ContentError::NotFound:
        return QStringLiteral("NOT_FOUND");
    case application::ContentError::Storage:
        return QStringLiteral("STORAGE");
    case application::ContentError::Integrity:
        return QStringLiteral("INTEGRITY");
    case application::ContentError::Cancelled:
        return QStringLiteral("CANCELLED");
    case application::ContentError::Conflict:
        return QStringLiteral("CONFLICT");
    }
    return QStringLiteral("UNKNOWN");
}

void ContentMarketPage::updateSearchState()
{
    const bool busy = m_snapshot.phase == application::ContentPhase::Searching
        || m_snapshot.phase == application::ContentPhase::LoadingVersions
        || m_snapshot.phase == application::ContentPhase::Downloading
        || m_snapshot.phase == application::ContentPhase::Cancelling;
    const bool fileBusy = m_snapshot.phase == application::ContentPhase::Downloading
        || m_snapshot.phase == application::ContentPhase::Cancelling;
    const bool hasController = m_controller != nullptr;
    const bool editable = hasController && !fileBusy;
    m_searchText->setEnabled(editable);
    m_gameVersion->setEnabled(editable);
    m_loader->setEnabled(editable);
    m_searchButton->setEnabled(hasController && !busy);
    m_results->setEnabled(hasController && !busy && !m_snapshot.page.projects.empty());
    m_versions->setEnabled(hasController && !busy
                           && !m_snapshot.versions.empty()
                           && m_snapshot.selectedProject.has_value());
    const int limit = m_snapshot.page.limit > 0 ? m_snapshot.page.limit : 20;
    m_previousPage->setEnabled(hasController && !busy && m_snapshot.page.offset > 0);
    m_nextPage->setEnabled(hasController && !busy
                           && m_snapshot.page.offset + limit < m_snapshot.page.total);
}

void ContentMarketPage::updateResults()
{
    const int count = static_cast<int>(m_snapshot.page.projects.size());
    const int offset = m_snapshot.page.offset;
    const int total = m_snapshot.page.total;
    QString summary;
    if (m_snapshot.phase == application::ContentPhase::Searching) {
        summary = QStringLiteral("正在搜索 Modrinth…");
    } else if (total > 0) {
        summary = QStringLiteral("找到 %1 个项目 · 第 %2–%3 项")
                      .arg(total)
                      .arg(offset + 1)
                      .arg(offset + count);
    } else if (m_snapshot.phase == application::ContentPhase::Ready
               || m_snapshot.phase == application::ContentPhase::Idle) {
        summary = m_snapshot.phase == application::ContentPhase::Idle
            ? QStringLiteral("输入条件后开始搜索。")
            : QStringLiteral("没有找到匹配项目，可以调整筛选条件后重试。");
    } else {
        summary = QStringLiteral("暂时没有搜索结果。");
    }
    m_resultsSummary->setText(summary);
    m_pageLabel->setText(total > 0
                             ? QStringLiteral("%1–%2 / %3").arg(offset + 1).arg(offset + count).arg(total)
                             : QStringLiteral("没有结果"));

    const QSignalBlocker blocker(m_results);
    m_results->clear();
    const QString selectedId = m_snapshot.selectedProject
        ? textFromStd(m_snapshot.selectedProject->id)
        : QString();
    for (const core::ContentProject& project : m_snapshot.page.projects) {
        const QString id = textFromStd(project.id);
        const QString title = textFromStd(project.title);
        const QString description = textFromStd(project.description);
        auto* item = new QListWidgetItem(
            description.isEmpty() ? title : title + QLatin1Char('\n') + description,
            m_results);
        item->setData(Qt::UserRole, id);
        item->setToolTip(description);
        item->setSizeHint(QSize(0, 54));
        if (id == selectedId) {
            m_results->setCurrentItem(item);
        }
    }
}

void ContentMarketPage::updateProject()
{
    const core::ContentProject* project = m_snapshot.selectedProject
        ? &m_snapshot.selectedProject.value()
        : nullptr;
    if (!project) {
        m_projectTitle->setText(QStringLiteral("选择一个项目"));
        m_projectDescription->setText(
            QStringLiteral("从左侧搜索结果中选择项目，查看介绍、版本和必需前置信息。"));
        m_projectMeta->clear();
        m_filterLabel->setText(appliedFilterText());
        m_versionsTitle->setText(QStringLiteral("版本列表"));
        return;
    }

    m_projectTitle->setText(textFromStd(project->title));
    m_projectDescription->setText(textFromStd(project->description));
    m_projectMeta->setText(
        QStringLiteral("来源：Modrinth · %1 次下载")
            .arg(QLocale().toString(static_cast<qulonglong>(project->downloads))));
    m_filterLabel->setText(appliedFilterText());
    m_versionsTitle->setText(
        QStringLiteral("版本列表 · %1").arg(textFromStd(project->title)));
}

void ContentMarketPage::updateVersions()
{
    const QSignalBlocker blocker(m_versions);
    m_versions->clear();
    for (const core::ContentVersion& version : m_snapshot.versions) {
        QString title = textFromStd(version.name);
        const QString number = textFromStd(version.number);
        if (title.isEmpty()) {
            title = number;
        } else if (!number.isEmpty() && number != title) {
            title += QStringLiteral(" · ") + number;
        }
        QStringList tags;
        if (!version.gameVersions.empty()) {
            QStringList versions;
            for (const std::string& value : version.gameVersions) {
                versions.append(textFromStd(value));
            }
            tags.append(QStringLiteral("MC %1").arg(versions.join(QStringLiteral(", "))));
        }
        if (!version.loaders.empty()) {
            QStringList loaders;
            for (const std::string& value : version.loaders) {
                loaders.append(loaderText(value));
            }
            tags.append(loaders.join(QStringLiteral(" / ")));
        }
        if (!version.releaseType.empty()) {
            tags.append(releaseTypeText(version.releaseType));
        }
        const QString detail = tags.isEmpty() ? QString() : tags.join(QStringLiteral(" · "));
        auto* item = new QListWidgetItem(
            detail.isEmpty() ? title : title + QLatin1Char('\n') + detail,
            m_versions);
        item->setData(Qt::UserRole, textFromStd(version.id));
        item->setSizeHint(QSize(0, 46));
    }

    bool selectionFound = false;
    for (int row = 0; row < m_versions->count(); ++row) {
        if (m_versions->item(row)->data(Qt::UserRole).toString() == m_selectedVersionId) {
            m_versions->setCurrentRow(row);
            selectionFound = true;
            break;
        }
    }
    if (!selectionFound && m_snapshot.phase != application::ContentPhase::Downloading
        && m_snapshot.phase != application::ContentPhase::Cancelling) {
        m_selectedVersionId.clear();
        m_versions->setCurrentRow(-1);
    }
    updateDependency();
}

void ContentMarketPage::updateDependency()
{
    const core::ContentVersion* version = selectedVersion();
    if (!version) {
        if (m_snapshot.phase == application::ContentPhase::LoadingVersions) {
            m_dependency->setText(QStringLiteral("正在加载版本…"));
        } else {
            m_dependency->setText(
                m_snapshot.versions.empty()
                    ? QStringLiteral("选择项目后加载版本；下载前会显示所选版本的必需前置信息。")
                    : QStringLiteral("选择一个版本后显示必需前置信息。"));
        }
        return;
    }

    QStringList required;
    for (const core::ContentDependency& dependency : version->dependencies) {
        if (dependency.type != "required") {
            continue;
        }
        const QString projectId = textFromStd(dependency.projectId);
        const QString versionId = textFromStd(dependency.versionId);
        if (!projectId.isEmpty() && !versionId.isEmpty()) {
            required.append(QStringLiteral("项目 %1 · 版本 %2").arg(projectId, versionId));
        } else if (!projectId.isEmpty()) {
            required.append(QStringLiteral("项目 %1").arg(projectId));
        }
    }
    if (required.isEmpty()) {
        m_dependency->setText(QStringLiteral("所选版本未声明必需前置；启动器不会自动下载前置。"));
    } else {
        m_dependency->setText(
            QStringLiteral("必需前置：%1。启动器不会自动下载前置。")
                .arg(required.join(QStringLiteral("；"))));
    }
}

void ContentMarketPage::updateActionBar()
{
    const auto phase = m_snapshot.phase;
    const core::ContentVersion* version = selectedVersion();
    const bool busy = phase == application::ContentPhase::Searching
        || phase == application::ContentPhase::LoadingVersions
        || phase == application::ContentPhase::Downloading
        || phase == application::ContentPhase::Cancelling;

    QString title;
    QString note;
    QString status;
    switch (phase) {
    case application::ContentPhase::Idle:
        title = QStringLiteral("模组市场");
        note = QStringLiteral("搜索 Modrinth，下载只会保存文件。");
        status = QStringLiteral("输入关键词或筛选条件后点击“搜索”。");
        break;
    case application::ContentPhase::Searching:
        title = QStringLiteral("正在搜索");
        note = QStringLiteral("正在从 Modrinth 获取搜索结果。");
        status = QStringLiteral("正在搜索…");
        break;
    case application::ContentPhase::LoadingVersions:
        title = QStringLiteral("正在加载版本");
        note = QStringLiteral("正在获取所选项目的版本列表。");
        status = QStringLiteral("正在加载版本…");
        break;
    case application::ContentPhase::Ready:
        title = version ? QStringLiteral("已选择版本") : QStringLiteral("选择版本以下载");
        note = QStringLiteral("下载只保存文件；安装前请确认加载器与游戏版本匹配。");
        status = version ? QStringLiteral("点击“下载文件”开始下载。")
                         : QStringLiteral("选择一个项目，再选择要下载的版本。");
        break;
    case application::ContentPhase::Downloading:
        title = QStringLiteral("正在下载");
        note = QStringLiteral("正在下载并校验所选文件，完成前请保持启动器打开。");
        status = QStringLiteral("正在下载并校验…");
        break;
    case application::ContentPhase::Cancelling:
        title = QStringLiteral("正在取消下载");
        note = QStringLiteral("正在等待下载任务确认取消。");
        status = QStringLiteral("正在取消…");
        break;
    case application::ContentPhase::Downloaded:
        title = QStringLiteral("下载完成");
        note = QStringLiteral("文件已保存；不会自动安装或修改当前游戏。");
        if (m_snapshot.downloadPath.isEmpty()) {
            status = QStringLiteral("下载完成，文件已保存。");
        } else {
            status = m_snapshot.reused
                ? QStringLiteral("已下载并复用本地校验文件：%1").arg(m_snapshot.downloadPath)
                : QStringLiteral("已下载：%1").arg(m_snapshot.downloadPath);
        }
        break;
    case application::ContentPhase::Cancelled:
        title = QStringLiteral("已取消下载");
        note = QStringLiteral("没有修改当前游戏，可以重新选择版本下载。");
        status = QStringLiteral("下载已取消。");
        break;
    case application::ContentPhase::Failed:
        title = QStringLiteral("操作未完成");
        note = errorText(m_snapshot.error)
            + QStringLiteral(" 没有修改当前游戏；可以调整条件或稍后重试。");
        status = QStringLiteral("操作未完成。");
        break;
    }
    m_actionBar->setTitle(title);
    m_actionBar->setNote(note);
    m_status->setText(status);
    m_status->setToolTip(m_snapshot.downloadPath);

    const bool canDownload = m_controller != nullptr && version != nullptr && !busy;
    if (canDownload) {
        QString primary = QStringLiteral("下载文件");
        if (phase == application::ContentPhase::Failed) {
            primary = QStringLiteral("重试下载");
        } else if (phase == application::ContentPhase::Cancelled) {
            primary = QStringLiteral("重新下载");
        } else if (phase == application::ContentPhase::Downloaded) {
            primary = QStringLiteral("再次下载");
        }
        m_actionBar->setPrimary(primary, true,
                                QStringLiteral("下载所选版本的文件；不会修改当前游戏。"));
        m_actionBar->setPrimaryVisible(true);
    } else {
        m_actionBar->setPrimary(QString(), false, {});
        m_actionBar->setPrimaryVisible(false);
    }

    if (phase == application::ContentPhase::Downloading) {
        m_actionBar->setSecondary(
            QStringLiteral("取消下载"), true, true,
            QStringLiteral("取消当前下载；已校验的缓存文件会保留。"), true);
    } else if (phase == application::ContentPhase::Cancelling) {
        m_actionBar->setSecondary(
            QStringLiteral("正在取消…"), true, false,
            QStringLiteral("等待当前下载任务确认取消。"), true);
    } else if (phase == application::ContentPhase::Downloaded
               && !m_snapshot.downloadPath.isEmpty()) {
        m_actionBar->setSecondary(
            QStringLiteral("打开下载文件夹"), true, true,
            QStringLiteral("打开保存本次下载文件的文件夹。"), false);
    } else {
        m_actionBar->setSecondary(QString(), false, false, {});
    }
    m_actionBar->setErrorState(m_snapshot.error != application::ContentError::None);
}

void ContentMarketPage::handleSecondary()
{
    if (m_snapshot.phase == application::ContentPhase::Downloading) {
        if (m_controller) {
            m_controller->cancel(m_snapshot.taskId);
        }
        return;
    }
    if (m_snapshot.phase == application::ContentPhase::Downloaded
        && !m_snapshot.downloadPath.isEmpty()) {
        emit openDownloadFolderRequested(m_snapshot.downloadPath);
    }
}

} // namespace hustmcl::ui
