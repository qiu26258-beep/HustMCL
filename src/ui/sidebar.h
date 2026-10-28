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

    void setRealMode(bool realMode);
    void setMarketAvailable(bool available);
    void setAccount(const application::AccountSnapshot& account, bool offlineMode);
    void setCurrentPage(int pageIndex);
    void setArtworkVisible(bool visible);
    void setCompact(bool compact);
    bool isRealMode() const;

signals:
    void pageActivated(int pageIndex);
    void accountSettingsRequested();
    void unimplementedFeatureRequested(const QString& feature);

private:
    void updateArtworkVisibility();
    void updateAccountLink();
    void updateAccountPresentation();
    void applyModeCopy();
    void updateMarketItem();

    QListWidget* m_nav = nullptr;
    QLabel* m_badge = nullptr;
    QLabel* m_avatar = nullptr;
    QLabel* m_accountName = nullptr;
    QLabel* m_accountState = nullptr;
    QLabel* m_navNote = nullptr;
    QLabel* m_accountNote = nullptr;
    QLabel* m_footer = nullptr;
    QPushButton* m_accountLink = nullptr;
    CampusArtwork* m_artwork = nullptr;
    QLabel* m_caption = nullptr;
    QLabel* m_subCaption = nullptr;
    application::AccountSnapshot m_account;
    bool m_realMode = false;
    bool m_offlineMode = true;
    bool m_marketAvailable = false;
    bool m_artworkRequested = false;
    bool m_compact = false;
};

} // namespace hustmcl::ui
