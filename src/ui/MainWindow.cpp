#include <windows.h>

#include "MainWindow.h"
#include "ProfileEditDialog.h"
#include "SettingsDialog.h"
#include "AboutDialog.h"
#include "ToggleSwitch.h"
#include "ActiveIndicatorDelegate.h"
#include "StyleSheet.h"

#include "core/ProfileStore.h"
#include "core/IPAssistantCore.h"
#include "core/LocaleManager.h"
#include "core/NetworkAdapter.h"

#include <QApplication>
#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>
#include <QDebug>

namespace {

NetworkAdapterBinding resolveProfileAdapter(const Profile& profile)
{
    return NetworkAdapter::resolve(profile.deviceGuid, profile.deviceName);
}

QIcon loadEmbeddedIcon(int resId)
{
    HICON hIcon = (HICON)LoadImageW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(resId),
        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    if (hIcon) {
        QIcon icon(QPixmap::fromImage(QImage::fromHICON(hIcon)));
        DestroyIcon(hIcon);
        return icon;
    }
    return {};
}

QIcon loadAppIcon()
{
    QIcon icon = loadEmbeddedIcon(1);
    if (!icon.isNull()) return icon;
    // Fallback: programmatic icon
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#3b8ee0"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 60, 60);
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(20);
    p.setFont(f);
    p.drawText(pm.rect(), Qt::AlignCenter, "IP");
    return pm;
}

// Small blue vertical bar used as active-profile indicator in the tray
QIcon makeActiveBarIcon()
{
    const int s = 16;
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#3b8ee0"));
    p.setPen(Qt::NoPen);
    p.drawRect(6, 1, 3, s - 2);
    return pm;
}

// Strict comparison between a currently-read config and a stored profile
bool configMatchesProfile(const Profile& current, const Profile& profile)
{
    if (!current.deviceGuid.isEmpty() && !profile.deviceGuid.isEmpty()) {
        if (current.deviceGuid.compare(profile.deviceGuid, Qt::CaseInsensitive) != 0)
            return false;
    } else if (current.deviceName.compare(
                   profile.deviceName, Qt::CaseInsensitive) != 0) {
        return false;
    }
    if (current.dhcp != profile.dhcp) return false;
    if (!current.dhcp) {
        if (current.ipAddress != profile.ipAddress) return false;
        if (current.subnetMask != profile.subnetMask) return false;
        if (!profile.gateway.isEmpty() && current.gateway != profile.gateway) return false;
    }
    if (current.defaultDns != profile.defaultDns) return false;
    if (!current.defaultDns) {
        if (current.preferredDns != profile.preferredDns) return false;
        if (current.alternateDns != profile.alternateDns) return false;
    }
    // IPv6 match
    if (current.ipv6Dhcp != profile.ipv6Dhcp) return false;
    if (!current.ipv6Dhcp) {
        if (current.ipv6Address != profile.ipv6Address) return false;
        if (current.ipv6Prefix != profile.ipv6Prefix) return false;
        if (!profile.ipv6Gateway.isEmpty() && current.ipv6Gateway != profile.ipv6Gateway) return false;
    }
    if (current.ipv6DefaultDns != profile.ipv6DefaultDns) return false;
    if (!current.ipv6DefaultDns) {
        if (current.ipv6PreferredDns != profile.ipv6PreferredDns) return false;
        if (current.ipv6AlternateDns != profile.ipv6AlternateDns) return false;
    }
    return true;
}

constexpr const char* kPersistedFileName = "profiles.json";
constexpr const char* kSettingsKeyMinimizeToTray = "ui/minimizeToTray";
constexpr const char* kSettingsKeyStartWithWindows = "ui/startWithWindows";

} // namespace

MainWindow::MainWindow(ProfileStore& store, const QStringList& adapters, QWidget* parent)
    : QMainWindow(parent)
    , m_store(store)
    , m_adapters(adapters)
{
    setWindowTitle(QStringLiteral("IP Assistant"));
    setWindowIcon(loadAppIcon());
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    resize(420, 540);
    setMinimumSize(380, 500);

    m_persistedFilePath = QApplication::applicationDirPath() + "/data/" + kPersistedFileName;

    setupUi(this);

    // The custom title bar and body must occupy the complete frameless window.
    setContentsMargins(0, 0, 0, 0);
    centralwidget->setContentsMargins(0, 0, 0, 0);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Override any outer padding/margin accidentally introduced by the global QSS.
    setStyleSheet(QString::fromUtf8(StyleSheet::kApp) + QStringLiteral(R"QSS(
QMainWindow#MainWindow {
    margin: 0px;
    padding: 0px;
    border: none;
}
QWidget#CentralWidget {
    margin: 0px;
    padding: 0px;
    border: none;
}
)QSS"));

    // Only the blue custom title bar is draggable.
    topBar->installEventFilter(this);
    iconLabel->installEventFilter(this);
    m_titleLabel->installEventFilter(this);

    // Stretch factors
    bodyLayout->setStretchFactor(leftCol, 1);
    bodyLayout->setStretchFactor(rightCol, 2);
    leftLay->setStretchFactor(m_profileList, 1);

    // Top bar icon
    iconLabel->setPixmap(
        loadAppIcon().pixmap(24, 24));

    // Tooltips
    m_settingsBtn->setToolTip(tr("Settings"));
    m_aboutBtn->setToolTip(tr("About"));
    m_closeBtn->setToolTip(tr("Close (minimize to tray)"));
    m_addBtn->setToolTip(tr("Add Profile"));
    m_delBtn->setToolTip(tr("Delete Profile"));
    m_editBtn->setToolTip(tr("Edit Profile"));

    // Cursors for tool buttons (activate btn cursor set in .ui)
    for (auto* b : { m_addBtn, m_delBtn, m_editBtn }) {
        b->setCursor(Qt::PointingHandCursor);
    }
    m_activateBtn->setStyleSheet("text-align: center;");

    // Active indicator delegate for profile list
    auto* indicatorDelegate = new ActiveIndicatorDelegate(this);
    m_profileList->setItemDelegate(indicatorDelegate);

    // Populate device combo
    m_deviceCombo->addItems(m_adapters);
    if (m_adapters.isEmpty()) {
        m_deviceCombo->addItem(tr("(no adapter detected)"));
        m_deviceCombo->setEnabled(false);
    }


    // Wire signals
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);
    connect(m_aboutBtn,    &QPushButton::clicked, this, &MainWindow::onOpenAbout);
    connect(m_closeBtn,    &QPushButton::clicked, this, [this] { hide(); });
    connect(m_profileList, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onProfileSelectionChanged);
    connect(m_addBtn,    &QPushButton::clicked, this, &MainWindow::onAddProfile);
    connect(m_delBtn,    &QPushButton::clicked, this, &MainWindow::onDeleteProfile);
    connect(m_editBtn,   &QPushButton::clicked, this, &MainWindow::onEditProfile);
    connect(m_activateBtn,&QPushButton::clicked, this, &MainWindow::onActivate);

    // Right-side form is read-only
    m_dhcpBox->setEnabled(false);
    m_deviceCombo->setEnabled(false);
    m_defaultDnsBox->setEnabled(false);
    for (auto* le : { m_ipEdit, m_maskEdit, m_gwEdit, m_prefDnsEdit, m_altDnsEdit }) {
        le->setReadOnly(true);
    }

    // --- IPv4 / IPv6 tab widget ---
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: none; background: #fafbfc; }"
        "QTabBar::tab { background: #e8ecf0; color: #444444; padding: 5px 12px;"
        "border: 1px solid #d0d0d0; border-bottom: none;"
        "border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background: #fafbfc; color: #3b8ee0; font-weight: bold; }"
        "QTabBar::tab:hover { background: #dce4ec; }");
    const int ipIdx = rightLay->indexOf(m_ipGroup);

    // Move DHCP switch from rightLay into IPv4 tab
    rightLay->removeWidget(m_dhcpBox);
    rightLay->removeWidget(m_ipGroup);
    rightLay->removeWidget(m_dnsGroup);

    // Delete section labels — not needed in tab layout; Qt auto-removes from layout on destruction
    delete m_ipSectionLabel;
    m_ipSectionLabel = nullptr;
    delete m_dnsSectionLabel;
    m_dnsSectionLabel = nullptr;

    // IPv4 tab — reuse existing group boxes
    auto* v4Page = new QWidget();
    auto* v4Lay = new QVBoxLayout(v4Page);
    v4Lay->setContentsMargins(0, 4, 0, 0);
    v4Lay->setSpacing(4);
    v4Lay->addWidget(m_dhcpBox);
    v4Lay->addWidget(m_ipGroup);
    v4Lay->addWidget(m_dnsGroup);
    m_tabWidget->addTab(v4Page, tr("IPv4"));

    // IPv6 tab — use same QGroupBox as IPv4 for identical styling
    auto* v6Page = new QWidget();
    auto* v6Lay = new QVBoxLayout(v6Page);
    v6Lay->setContentsMargins(0, 4, 0, 0);
    v6Lay->setSpacing(4);

    m_ipv6DhcpBox = new ToggleSwitch(tr("DHCP"));
    m_ipv6DhcpBox->setChecked(true);
    m_ipv6DhcpBox->setEnabled(false);
    m_ipv6DhcpBox->setMinimumSize(0, 26);
    v6Lay->addWidget(m_ipv6DhcpBox);

    auto* v6IpBox = new QGroupBox();
    v6IpBox->setObjectName("IpGroup");
    v6IpBox->setFlat(true);
    v6IpBox->setStyleSheet("QGroupBox#IpGroup { border: none; margin: 0px; padding: 0px; }");
    auto* v6IpLay = new QVBoxLayout(v6IpBox);
    v6IpLay->setContentsMargins(0, 0, 0, 0);
    v6IpLay->setSpacing(4);

    m_ipv6AddrLbl = new QLabel(tr("IPv6 Address"));
    m_ipv6AddrLbl->setContentsMargins(0, 0, 0, 0);
    v6IpLay->addWidget(m_ipv6AddrLbl);
    m_ipv6AddressEdit = new QLineEdit();
    m_ipv6AddressEdit->setReadOnly(true);
    v6IpLay->addWidget(m_ipv6AddressEdit);

    m_ipv6PrefLbl = new QLabel(tr("Prefix Length"));
    m_ipv6PrefLbl->setContentsMargins(0, 0, 0, 0);
    v6IpLay->addWidget(m_ipv6PrefLbl);
    m_ipv6PrefixEdit = new QLineEdit();
    m_ipv6PrefixEdit->setReadOnly(true);
    v6IpLay->addWidget(m_ipv6PrefixEdit);

    m_ipv6GwLbl = new QLabel(tr("Default Gateway"));
    m_ipv6GwLbl->setContentsMargins(0, 0, 0, 0);
    v6IpLay->addWidget(m_ipv6GwLbl);
    m_ipv6GatewayEdit = new QLineEdit();
    m_ipv6GatewayEdit->setReadOnly(true);
    v6IpLay->addWidget(m_ipv6GatewayEdit);
    v6Lay->addWidget(v6IpBox);

    auto* v6DnsBox = new QGroupBox();
    v6DnsBox->setObjectName("DnsGroup");
    v6DnsBox->setFlat(true);
    v6DnsBox->setStyleSheet("QGroupBox#DnsGroup { border: none; margin: 0px; padding: 0px; }");
    auto* v6DnsLay = new QVBoxLayout(v6DnsBox);
    v6DnsLay->setContentsMargins(0, 0, 0, 0);
    v6DnsLay->setSpacing(4);

    m_ipv6DefaultDnsBox = new ToggleSwitch(tr("Automatic DNS"));
    m_ipv6DefaultDnsBox->setChecked(true);
    m_ipv6DefaultDnsBox->setEnabled(false);
    m_ipv6DefaultDnsBox->setMinimumSize(0, 26);
    v6DnsLay->addWidget(m_ipv6DefaultDnsBox);

    m_ipv6PdnsLbl = new QLabel(tr("Preferred DNS"));
    m_ipv6PdnsLbl->setContentsMargins(0, 0, 0, 0);
    v6DnsLay->addWidget(m_ipv6PdnsLbl);
    m_ipv6PrefDnsEdit = new QLineEdit();
    m_ipv6PrefDnsEdit->setReadOnly(true);
    v6DnsLay->addWidget(m_ipv6PrefDnsEdit);

    m_ipv6AdnsLbl = new QLabel(tr("Alternate DNS"));
    m_ipv6AdnsLbl->setContentsMargins(0, 0, 0, 0);
    v6DnsLay->addWidget(m_ipv6AdnsLbl);
    m_ipv6AltDnsEdit = new QLineEdit();
    m_ipv6AltDnsEdit->setReadOnly(true);
    v6DnsLay->addWidget(m_ipv6AltDnsEdit);
    v6Lay->addWidget(v6DnsBox);

    m_tabWidget->addTab(v6Page, tr("IPv6"));
    rightLay->insertWidget(ipIdx, m_tabWidget);

    // Uniform line-edit height (reduce from 28 to 22)
    for (auto* le : findChildren<QLineEdit*>())
        le->setMinimumHeight(22);

    buildTray();
    detectActiveProfile();
    populateProfileList();

    connect(&m_store, &ProfileStore::changed, this, &MainWindow::populateProfileList);

    connect(&m_store, &ProfileStore::changed, this, [this] {
        QString err;
        if (!m_store.save(m_persistedFilePath, &err)) {
            showNotification(tr("Save failed: %1").arg(err),
                             QSystemTrayIcon::Warning, 4000);
        }
    });

    connect(&LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &MainWindow::retranslateUi);

    onProfileSelectionChanged();
}


MainWindow::~MainWindow() = default;

void MainWindow::showNotification(const QString& message,
                                  QSystemTrayIcon::MessageIcon icon,
                                  int timeoutMs)
{
    if (m_tray && m_tray->isVisible()) {
        m_tray->showMessage(tr("IP Assistant"), message, icon, timeoutMs);
        return;
    }

    switch (icon) {
    case QSystemTrayIcon::Critical:
        qCritical().noquote() << message;
        break;
    case QSystemTrayIcon::Warning:
        qWarning().noquote() << message;
        break;
    default:
        qInfo().noquote() << message;
        break;
    }
}

void MainWindow::buildTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = nullptr;
        return;
    }

    m_tray = new QSystemTrayIcon(loadAppIcon(), this);
    m_tray->setToolTip(tr("IP Assistant"));

    m_trayMenu = new QMenu(this);

    // Theme-aware palette colors (used for QSS + icons below)
    QPalette pal = m_trayMenu->palette();
    QColor bg = pal.color(QPalette::Window);
    QColor fg = pal.color(QPalette::WindowText);
    {
        QColor selBg = bg.lightness() < 128 ? bg.lighter(130) : bg.darker(120);
        m_trayMenu->setStyleSheet(QStringLiteral(
            "QMenu { background: %1; color: %2; border: 1px solid %3; padding: 2px; }"
            "QMenu::item { color: %2; padding: 6px 24px; }"
            "QMenu::item:selected { background: %4; }"
            "QMenu::separator { background: %3; height: 1px; margin: 3px 6px; }"
        ).arg(bg.name(), fg.name(), selBg.name(), selBg.name()));
    }

        auto* exitAct = m_trayMenu->addAction(loadEmbeddedIcon(4), tr("Exit"));
    exitAct->setData(QString()); // mark as non-profile action

    connect(m_trayMenu, &QMenu::triggered, this, &MainWindow::onTrayMenuAction);
    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_tray->show();
}

void MainWindow::rebuildTrayMenu()
{
    if (!m_trayMenu) return;

    // Remove all previously-added submenus
    QList<QMenu*> oldMenus;
    for (QAction* a : m_trayMenu->actions()) {
        QMenu* sub = a->menu();
        if (sub && sub->property("adapterMenu").toBool())
            oldMenus.append(sub);
    }
    for (QMenu* sub : oldMenus) {
        m_trayMenu->removeAction(sub->menuAction());
        delete sub;
    }

    QAction* exitAction = nullptr;
    for (QAction* a : m_trayMenu->actions()) {
        if (a->text() == tr("Exit")) {
            exitAction = a;
            break;
        }
    }

    // Group profiles by the adapter's current display name.
    QMap<QString, QList<const Profile*>> groups;
    for (const auto& p : m_store.profiles()) {
        const NetworkAdapterBinding binding = resolveProfileAdapter(p);
        const QString displayName = binding.isValid()
            ? binding.name
            : tr("%1 (Unavailable)").arg(p.deviceName);
        groups[displayName].append(&p);
    }

    // Submenu style: use system Window/WindowText (dark mode = dark bg + light text)
    QPalette pal = m_trayMenu->palette();
    QColor bg = pal.color(QPalette::Window);
    QColor fg = pal.color(QPalette::WindowText);
    QColor selBg = bg.lightness() < 128 ? bg.lighter(130) : bg.darker(120);
    const QString subStyle = QStringLiteral(
        "QMenu { background: %1; color: %2; border: 1px solid %3; padding: 2px; }"
        "QMenu::item { color: %2; padding: 5px 24px; }"
        "QMenu::item:selected { background: %4; }"
    ).arg(bg.name(), fg.name(), selBg.name(), selBg.name());

    const QIcon wiredIcon = loadEmbeddedIcon(2);
    const QIcon wifiIcon = loadEmbeddedIcon(3);
    const QIcon barIcon = makeActiveBarIcon();

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QString& adapterName = it.key();
        const auto& profiles = it.value();

        bool isWifi = adapterName.contains("Wi-Fi", Qt::CaseInsensitive)
                    || adapterName.contains("Wireless", Qt::CaseInsensitive)
                    || adapterName.contains("WLAN", Qt::CaseInsensitive);

        QMenu* adapterMenu = new QMenu(adapterName, m_trayMenu);
        adapterMenu->setIcon(isWifi ? wifiIcon : wiredIcon);
        adapterMenu->setProperty("adapterMenu", true);
        adapterMenu->setStyleSheet(subStyle);

        for (const Profile* p : profiles) {
            QAction* act = adapterMenu->addAction(p->name);
            act->setData(p->name);
            if (p->name == m_activeProfileName)
                act->setIcon(barIcon);
            QStringList tip;
            if (p->dhcp) {
                tip << tr("DHCP");
            } else {
                tip << tr("IP: %1").arg(p->ipAddress);
                tip << tr("Mask: %1").arg(p->subnetMask);
                if (!p->gateway.isEmpty())
                    tip << tr("Gateway: %1").arg(p->gateway);
            }
            if (!p->defaultDns) {
                if (!p->preferredDns.isEmpty()) {
                    QString dns = p->preferredDns;
                    if (!p->alternateDns.isEmpty())
                        dns += QStringLiteral(", ") + p->alternateDns;
                    tip << tr("DNS: %1").arg(dns);
                }
            } else {
                tip << tr("DNS: DHCP");
            }
            act->setToolTip(tip.join(QStringLiteral("\n")));
        }

        connect(adapterMenu, &QMenu::triggered, this, &MainWindow::onTrayMenuAction);

        if (exitAction)
            m_trayMenu->insertMenu(exitAction, adapterMenu);
        else
            m_trayMenu->addMenu(adapterMenu);
    }
}

void MainWindow::detectActiveProfile()
{
    m_activeProfileName.clear();
    QMap<QString, Profile> cache;
    for (const auto& p : m_store.profiles()) {
        const NetworkAdapterBinding binding = resolveProfileAdapter(p);
        if (!binding.isValid())
            continue;
        const QString cacheKey = binding.guid.isEmpty()
            ? binding.name
            : binding.guid.toLower();
        if (!cache.contains(cacheKey))
            cache[cacheKey] = NetworkAdapter::queryCurrentConfig(binding.name);
        if (configMatchesProfile(cache[cacheKey], p)) {
            m_activeProfileName = p.name;
            break;
        }
    }
}

void MainWindow::populateProfileList()
{
    QString prevName;
    if (auto* it = m_profileList->currentItem()) {
        prevName = it->text();
    }

    m_profileList->clear();
    for (const auto& p : m_store.profiles()) {
        auto* item = new QListWidgetItem(p.name);
        const NetworkAdapterBinding binding = resolveProfileAdapter(p);
        const QString deviceText = binding.isValid()
            ? binding.name
            : tr("%1 (Unavailable)").arg(p.deviceName);
        item->setToolTip(tr("Device: %1 | %2")
            .arg(deviceText, p.dhcp ? "DHCP" : "Static"));
        m_profileList->addItem(item);
    }

    rebuildTrayMenu();

    // Restore active indicator in delegate
    if (auto* d = qobject_cast<ActiveIndicatorDelegate*>(m_profileList->itemDelegate()))
        d->setActiveName(m_activeProfileName);

    if (!prevName.isEmpty()) {
        auto items = m_profileList->findItems(prevName, Qt::MatchExactly);
        if (!items.isEmpty()) {
            m_profileList->setCurrentItem(items.first());
            return;
        }
    }
    if (m_profileList->count() > 0) {
        m_profileList->setCurrentRow(0);
    } else {
        onProfileSelectionChanged();
    }
}

void MainWindow::onProfileSelectionChanged()
{
    auto* it = m_profileList->currentItem();
    if (!it) {
        m_ipEdit->clear();
        m_maskEdit->clear();
        m_gwEdit->clear();
        m_prefDnsEdit->clear();
        m_altDnsEdit->clear();
        m_deviceCombo->setCurrentIndex(0);
        m_dhcpBox->setChecked(true);
        m_defaultDnsBox->setChecked(false);
        m_ipGroup->setEnabled(false);
        m_dnsGroup->setEnabled(true);
        updateButtonsEnabled();
        return;
    }

    int idx = m_store.indexOf(it->text());
    if (idx < 0) return;
    loadProfileIntoForm(m_store.profiles()[idx]);
    updateButtonsEnabled();
}

void MainWindow::loadProfileIntoForm(const Profile& p)
{
    m_dhcpBox->setChecked(p.dhcp);
    const NetworkAdapterBinding binding = resolveProfileAdapter(p);
    const QString deviceText = binding.isValid()
        ? binding.name
        : tr("%1 (Unavailable)").arg(p.deviceName);
    int devIdx = m_deviceCombo->findText(deviceText);
    if (devIdx >= 0) m_deviceCombo->setCurrentIndex(devIdx);
    else if (!deviceText.isEmpty()) {
        m_deviceCombo->insertItem(0, deviceText);
        m_deviceCombo->setCurrentIndex(0);
    }

    m_ipEdit->setText(p.ipAddress);
    m_maskEdit->setText(p.subnetMask);
    m_gwEdit->setText(p.gateway);
    m_defaultDnsBox->setChecked(p.defaultDns);
    m_prefDnsEdit->setText(p.preferredDns);
    m_altDnsEdit->setText(p.alternateDns);

    m_ipGroup->setEnabled(!p.dhcp);
    m_dnsGroup->setEnabled(!p.defaultDns);

    // IPv6 fields
    m_ipv6DhcpBox->setChecked(p.ipv6Dhcp);
    m_ipv6AddressEdit->setText(p.ipv6Address);
    m_ipv6PrefixEdit->setText(p.ipv6Dhcp ? QString() : QString::number(p.ipv6Prefix));
    m_ipv6GatewayEdit->setText(p.ipv6Gateway);
    m_ipv6DefaultDnsBox->setChecked(p.ipv6DefaultDns);
    m_ipv6PrefDnsEdit->setText(p.ipv6PreferredDns);
    m_ipv6AltDnsEdit->setText(p.ipv6AlternateDns);
}

void MainWindow::updateButtonsEnabled()
{
    bool hasSel = m_profileList->currentItem() != nullptr;
    m_delBtn->setEnabled(hasSel);
    m_editBtn->setEnabled(hasSel);
    m_activateBtn->setEnabled(hasSel);
}

void MainWindow::onAddProfile()
{
    ProfileEditDialog dlg(m_store, m_adapters, this);
    dlg.setWindowTitle(tr("Add Profile"));
    if (dlg.exec() != QDialog::Accepted) return;

    Profile p = dlg.profile();
    QString reason;
    if (!p.isValid(&reason)) {
        QMessageBox::warning(this, tr("Invalid profile"), reason);
        return;
    }
    if (m_store.contains(p.name)) {
        QMessageBox::warning(this, tr("Duplicate"),
                             tr("A profile named '%1' already exists.").arg(p.name));
        return;
    }
    m_store.upsert(p);
    showNotification(tr("Profile '%1' added.").arg(p.name));
}

void MainWindow::onDeleteProfile()
{
    auto* it = m_profileList->currentItem();
    if (!it) return;
    QString name = it->text();
    auto btn = QMessageBox::question(
        this, tr("Delete profile"),
        tr("Delete profile '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;
    m_store.remove(name);
    showNotification(tr("Profile '%1' deleted.").arg(name));
}

void MainWindow::onEditProfile()
{
    auto* it = m_profileList->currentItem();
    if (!it) return;
    int idx = m_store.indexOf(it->text());
    if (idx < 0) return;
    const Profile& existing = m_store.profiles()[idx];

    ProfileEditDialog dlg(m_store, m_adapters, existing, this);
    dlg.setWindowTitle(tr("Edit Profile"));
    if (dlg.exec() != QDialog::Accepted) return;

    Profile p = dlg.profile();
    QString reason;
    if (!p.isValid(&reason)) {
        QMessageBox::warning(this, tr("Invalid profile"), reason);
        return;
    }
    m_store.upsert(p);
    showNotification(tr("Profile '%1' updated.").arg(p.name));
}

void MainWindow::onActivate()
{
    auto* it = m_profileList->currentItem();
    if (!it) return;
    int idx = m_store.indexOf(it->text());
    if (idx < 0) return;
    const Profile& p = m_store.profiles()[idx];

    if (!resolveProfileAdapter(p).isValid()) {
        const QString message = tr(
            "The network adapter '%1' is unavailable. "
            "It may have been removed, replaced, or disabled. "
            "Edit the profile to select another adapter.")
            .arg(p.deviceName);
        QMessageBox::warning(this, tr("Adapter unavailable"), message);
        showNotification(message, QSystemTrayIcon::Warning, 5000);
        return;
    }

    m_activateBtn->setEnabled(false);
    m_activateBtn->setText(tr("Activating..."));

    IPAssistantCore::Result r = IPAssistantCore::apply(p);

    m_activateBtn->setEnabled(true);
    m_activateBtn->setText(tr("Activate"));

    if (!r.ok) {
        QMessageBox::critical(this, tr("Activation failed"),
                              tr("Failed to apply '%1'.\n\n%2")
                                  .arg(p.name, r.summary));
        showNotification(tr("Failed: %1").arg(p.name),
                         QSystemTrayIcon::Critical, 5000);
    } else {
        m_activeProfileName = p.name;
        if (auto* d = qobject_cast<ActiveIndicatorDelegate*>(m_profileList->itemDelegate()))
            d->setActiveName(p.name);
        m_profileList->viewport()->update();
        rebuildTrayMenu();
        showNotification(r.summary, QSystemTrayIcon::Information, 5000);
    }
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QSettings qs;
    qs.setValue(kSettingsKeyStartWithWindows, dlg.startWithWindows());
    qs.setValue(kSettingsKeyMinimizeToTray,  dlg.minimizeToTray());
}

void MainWindow::onOpenAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        show();
        raise();
        activateWindow();
    } else if (reason == QSystemTrayIcon::Context) {
        if (m_trayMenu) {
            m_trayMenu->popup(QCursor::pos());
        }
    }
}

void MainWindow::onTrayMenuAction(QAction* action)
{
    if (!action) return;
    const QString text = action->text();
    if (text == tr("Exit")) { qApp->quit(); return; }

    QString profileName = action->data().toString();
    if (profileName.isEmpty()) return;
    int idx = m_store.indexOf(profileName);
    if (idx < 0) return;
    const Profile& profile = m_store.profiles()[idx];
    if (!resolveProfileAdapter(profile).isValid()) {
        if (m_tray) {
            m_tray->showMessage(
                tr("Adapter unavailable"),
                tr("The network adapter '%1' is unavailable. Edit the profile to rebind it.")
                    .arg(profile.deviceName),
                QSystemTrayIcon::Warning);
        }
        return;
    }
    IPAssistantCore::Result r = IPAssistantCore::apply(profile);
    if (!r.ok) {
        if (m_tray) m_tray->showMessage(tr("IP Assistant"), r.summary, QSystemTrayIcon::Critical);
    } else {
        m_activeProfileName = profileName;
        if (auto* d = qobject_cast<ActiveIndicatorDelegate*>(m_profileList->itemDelegate()))
            d->setActiveName(profileName);
        m_profileList->viewport()->update();
        rebuildTrayMenu();
        if (m_tray) m_tray->showMessage(tr("IP Assistant"), r.summary, QSystemTrayIcon::Information);
    }
}

void MainWindow::retranslateUi()
{
    m_titleLabel->setText(tr("IP ASSISTANT"));
    m_settingsBtn->setToolTip(tr("Settings"));
    m_aboutBtn->setToolTip(tr("About"));
    m_closeBtn->setToolTip(tr("Close (minimize to tray)"));

    m_profilesLabel->setText(tr("Profiles"));
    m_addBtn->setToolTip(tr("Add Profile"));
    m_delBtn->setToolTip(tr("Delete Profile"));
    m_editBtn->setToolTip(tr("Edit Profile"));
    m_activateBtn->setText(tr("Activate"));

    m_dhcpBox->setText(tr("DHCP"));
    m_deviceLabel->setText(tr("Device"));
    m_ipLabel->setText(tr("IP Address"));
    m_maskLabel->setText(tr("Subnet Mask"));
    m_gwLabel->setText(tr("Default Gateway"));
    m_defaultDnsBox->setText(tr("Automatic DNS"));
    m_prefDnsLabel->setText(tr("Preferred DNS"));
    m_altDnsLabel->setText(tr("Alternate DNS"));

    // IPv6 tab labels
    if (m_tabWidget) {
        m_tabWidget->setTabText(0, tr("IPv4"));
        m_tabWidget->setTabText(1, tr("IPv6"));
    }
    if (m_ipv6DhcpBox)   m_ipv6DhcpBox->setText(tr("DHCP"));
    if (m_ipv6AddrLbl)   m_ipv6AddrLbl->setText(tr("IPv6 Address"));
    if (m_ipv6PrefLbl)   m_ipv6PrefLbl->setText(tr("Prefix Length"));
    if (m_ipv6GwLbl)     m_ipv6GwLbl->setText(tr("Default Gateway"));
    if (m_ipv6DefaultDnsBox) m_ipv6DefaultDnsBox->setText(tr("Automatic DNS"));
    if (m_ipv6PdnsLbl)   m_ipv6PdnsLbl->setText(tr("Preferred DNS"));
    if (m_ipv6AdnsLbl)   m_ipv6AdnsLbl->setText(tr("Alternate DNS"));

    if (m_trayMenu) {
        m_trayMenu->clear();
        auto* showAct = m_trayMenu->addAction(tr("Show"), this, [this] { show(); raise(); activateWindow(); });
        showAct->setData(QString());
        m_trayMenu->addSeparator();
        m_trayMenu->addSeparator();
    auto* exitAct = m_trayMenu->addAction(loadEmbeddedIcon(4), tr("Exit"));
        exitAct->setData(QString());
        connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
        m_tray->setToolTip(tr("IP Assistant"));
        rebuildTrayMenu();
    }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    const bool isTitleArea =
        watched == topBar ||
        watched == iconLabel ||
        watched == m_titleLabel;

    if (isTitleArea && event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && windowHandle()) {
            windowHandle()->startSystemMove();
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    QSettings qs;
    const bool minToTray = qs.value(kSettingsKeyMinimizeToTray, true).toBool();
    if (m_tray && minToTray) {
        if (!isVisible()) {
            e->accept();
            return;
        }
        hide();
        m_tray->showMessage(tr("IP Assistant"),
                            tr("Running in the system tray. Right-click the icon to switch profiles."),
                            QSystemTrayIcon::Information, 3000);
        e->ignore();
        return;
    }
    QMainWindow::closeEvent(e);
}
