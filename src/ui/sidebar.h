#pragma once

#include "hustmcl/application/frontend_snapshot.h"

#include <QWidget>

class QFrame;
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
    void refreshTheme();
    bool isRealMode() const;

signals:
    void pageActivated(int pageIndex);
    void accountSettingsRequested();
    void unimplementedFeatureRequested(const QString& feature);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateArtworkVisibility();
    void updateAccountLink();
    void updateAccountPresentation();
    void applyModeCopy();
    void updateMarketItem();
    void refreshIcons();

    QLabel* m_brandMark = nullptr;
    QListWidget* m_nav = nullptr;
    QLabel* m_badge = nullptr;
    QLabel* m_avatar = nullptr;
    QLabel* m_accountName = nullptr;
    QLabel* m_accountState = nullptr;
    QLabel* m_navNote = nullptr;
    QLabel* m_accountNote = nullptr;
    QLabel* m_footer = nullptr;
    QFrame* m_mottoSeal = nullptr;
    QPushButton* m_accountLink = nullptr;
    CampusArtwork* m_artwork = nullptr;
    application::AccountSnapshot m_account;
    bool m_realMode = false;
    bool m_offlineMode = true;
    bool m_marketAvailable = false;
    bool m_artworkRequested = false;
    bool m_compact = false;
};

} // namespace hustmcl::ui
