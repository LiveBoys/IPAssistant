#ifndef IPSWITCH_MAIN_WINDOW_H
#define IPSWITCH_MAIN_WINDOW_H

#include "core/Profile.h"
#include "ui_MainWindow.h"

#include <QMainWindow>
#include <QStringList>
#include <QSystemTrayIcon>

class ProfileStore;
class QMenu;
class QCloseEvent;
class QEvent;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QTabWidget;
class ToggleSwitch;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(ProfileStore& store, const QStringList& adapters, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onProfileSelectionChanged();
    void onAddProfile();
    void onDeleteProfile();
    void onEditProfile();
    void onActivate();
    void onOpenSettings();
    void onOpenAbout();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuAction(QAction* action);

private:
    void buildTray();
    void populateProfileList();
    void loadProfileIntoForm(const Profile& p);
    void rebuildTrayMenu();
    void detectActiveProfile();
    void updateButtonsEnabled();
    void retranslateUi();
    void showNotification(const QString& message,
                          QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                          int timeoutMs = 3000);
    bool eventFilter(QObject* watched, QEvent* event) override;

    ProfileStore& m_store;
    QStringList    m_adapters;

    QSystemTrayIcon* m_tray = nullptr;
    QMenu*         m_trayMenu = nullptr;

    QString        m_persistedFilePath;
    QString        m_activeProfileName;
    QPoint         m_dragPos;

    // IPv6 tab widgets (created programmatically)
    QTabWidget*    m_tabWidget = nullptr;
    ToggleSwitch*  m_ipv6DhcpBox = nullptr;
    QLineEdit*     m_ipv6AddressEdit = nullptr;
    QLineEdit*     m_ipv6PrefixEdit = nullptr;
    QLineEdit*     m_ipv6GatewayEdit = nullptr;
    ToggleSwitch*  m_ipv6DefaultDnsBox = nullptr;
    QLineEdit*     m_ipv6PrefDnsEdit = nullptr;
    QLineEdit*     m_ipv6AltDnsEdit = nullptr;
    QLabel*        m_ipv6AddrLbl = nullptr;
    QLabel*        m_ipv6PrefLbl = nullptr;
    QLabel*        m_ipv6GwLbl = nullptr;
    QLabel*        m_ipv6PdnsLbl = nullptr;
    QLabel*        m_ipv6AdnsLbl = nullptr;
};

#endif // IPSWITCH_MAIN_WINDOW_H
