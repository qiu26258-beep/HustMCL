#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;

namespace hustmcl::ui {

class CampusArtwork;

class Sidebar : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);

    void setAccount(const application::AccountSnapshot& account);
    void setCurrentPage(int pageIndex);
    void setArtworkVisible(bool visible);
    void setCompact(bool compact);

signals:
    void pageActivated(int pageIndex);
    void unimplementedFeatureRequested(const QString& feature);

private:
    void updateArtworkVisibility();

    QListWidget* m_nav = nullptr;
    QLabel* m_avatar = nullptr;
    QLabel* m_accountName = nullptr;
    QLabel* m_accountState = nullptr;
    QPushButton* m_accountLink = nullptr;
    QLabel* m_navNote = nullptr;
    CampusArtwork* m_artwork = nullptr;
    QLabel* m_caption = nullptr;
    QLabel* m_subCaption = nullptr;
    bool m_artworkRequested = false;
    bool m_compact = false;
};

} // namespace hustmcl::ui
