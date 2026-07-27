#include "ProfileEditDialog.h"
#include "ToggleSwitch.h"
#include "core/ProfileStore.h"
#include "core/NetworkAdapter.h"
#include "core/Profile.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTabWidget>

ProfileEditDialog::ProfileEditDialog(ProfileStore& store,
                                     const QStringList& adapters,
                                     QWidget* parent)
    : QDialog(parent)
    , m_store(store)
    , m_adapters(adapters)
{
    setupUi(this);
    m_deviceCombo->addItems(m_adapters);
    if (m_adapters.isEmpty()) {
        m_deviceCombo->addItem(tr("(no adapter detected)"));
        m_deviceCombo->setEnabled(false);
    }
    setWindowTitle(tr("Add Profile"));

    init();

    // Auto-load current config when device changes (new profiles only).
    connect(m_deviceCombo, &QComboBox::currentTextChanged, this, [this](const QString& dev) {
        if (dev.isEmpty() || dev == tr("(no adapter detected)")) return;
        Profile cfg = NetworkAdapter::queryCurrentConfig(dev);
        if (cfg.ipAddress.isEmpty()) return;
        m_dhcpBox->setChecked(cfg.dhcp);
        m_ipEdit->setText(cfg.ipAddress);
        m_maskEdit->setText(cfg.subnetMask);
        m_gwEdit->setText(cfg.gateway);
        m_defaultDnsBox->setChecked(cfg.defaultDns);
        m_prefDnsEdit->setText(cfg.preferredDns);
        m_altDnsEdit->setText(cfg.alternateDns);
        bool staticIp = !cfg.dhcp;
        m_ipEdit->setEnabled(staticIp);
        m_maskEdit->setEnabled(staticIp);
        m_gwEdit->setEnabled(staticIp);
        bool customDns = !cfg.defaultDns;
        m_prefDnsEdit->setEnabled(customDns);
        m_altDnsEdit->setEnabled(customDns);
    });
}

ProfileEditDialog::ProfileEditDialog(ProfileStore& store,
                                     const QStringList& adapters,
                                     const Profile& existing,
                                     QWidget* parent)
    : QDialog(parent)
    , m_store(store)
    , m_adapters(adapters)
    , m_originalName(existing.name)
    , m_originalDeviceGuid(existing.deviceGuid)
{
    setupUi(this);
    m_deviceCombo->addItems(m_adapters);
    if (m_adapters.isEmpty()) {
        m_deviceCombo->addItem(tr("(no adapter detected)"));
        m_deviceCombo->setEnabled(false);
    }
    setWindowTitle(tr("Edit Profile"));

    init();
    loadFrom(existing);
}

void ProfileEditDialog::init()
{
    connect(m_dhcpBox, &ToggleSwitch::toggled, this, [this](bool on) {
        bool en = !on;
        m_ipEdit->setEnabled(en);
        m_maskEdit->setEnabled(en);
        m_gwEdit->setEnabled(en);
        if (on) {
            clearError(m_ipEdit, m_ipErrLabel);
            clearError(m_maskEdit, m_maskErrLabel);
            clearError(m_gwEdit, m_gwErrLabel);
        }
    });
    connect(m_defaultDnsBox, &ToggleSwitch::toggled, this, [this](bool on) {
        bool en = !on;
        m_prefDnsEdit->setEnabled(en);
        m_altDnsEdit->setEnabled(en);
        if (on) {
            clearError(m_prefDnsEdit, m_prefDnsErrLabel);
            clearError(m_altDnsEdit, m_altDnsErrLabel);
        }
    });

    auto installIpValidator = [this](QLineEdit* edit, QLabel* errLabel, bool optional) {
        connect(edit, &QLineEdit::textChanged, this, [this, edit, errLabel, optional](const QString& text) {
            if (!edit->isEnabled()) return;
            if (text.trimmed().isEmpty()) {
                if (optional) { clearError(edit, errLabel); return; }
                setError(edit, errLabel, tr("Required"));
                return;
            }
            if (!Profile::isValidIPv4(text.trimmed())) {
                setError(edit, errLabel, tr("Invalid IPv4"));
            } else {
                clearError(edit, errLabel);
            }
        });
    };
    installIpValidator(m_ipEdit,    m_ipErrLabel,    false);
    installIpValidator(m_maskEdit,  m_maskErrLabel,  false);
    installIpValidator(m_gwEdit,    m_gwErrLabel,    true);
    installIpValidator(m_prefDnsEdit, m_prefDnsErrLabel, true);
    installIpValidator(m_altDnsEdit, m_altDnsErrLabel,  true);

    // --- Move IPv4 form rows into a tab widget ---
    auto* tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #e0e6eb; border-top: none;"
        "border-radius: 0 0 4px 4px; background: #ffffff; }"
        "QTabBar::tab { background: #e8ecf0; color: #444; padding: 4px 16px;"
        "border: 1px solid #d0d0d0; border-bottom: none;"
        "border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background: #ffffff; color: #3b8ee0; font-weight: bold; }");

    // --- IPv4 tab ---
    auto* v4Page = new QWidget();
    auto* v4Form = new QFormLayout(v4Page);
    v4Form->setContentsMargins(8, 12, 8, 8);
    v4Form->setSpacing(6);

    // Move existing IPv4 rows from formLayout to v4Form
    for (int row = 2; row < 9; ++row) {
        QLayoutItem* labelItem = formLayout->itemAt(row, QFormLayout::LabelRole);
        QLayoutItem* fieldItem = formLayout->itemAt(row, QFormLayout::FieldRole);
        QWidget* labelW = labelItem ? labelItem->widget() : nullptr;
        QWidget* fieldW = fieldItem ? fieldItem->widget() : nullptr;
        if (labelW && fieldW)
            v4Form->addRow(labelW, fieldW);
    }
    // Remove old rows from formLayout (reverse order)
    for (int row = 8; row >= 2; --row)
        formLayout->removeRow(row);
    tabWidget->addTab(v4Page, tr("IPv4"));

    // --- IPv6 tab ---
    auto* v6Page = new QWidget();
    auto* v6Form = new QFormLayout(v6Page);
    v6Form->setContentsMargins(8, 12, 8, 8);
    v6Form->setSpacing(6);

    auto* ipv6DhcpLabel = new QLabel(tr("DHCP"));
    m_ipv6DhcpBox = new ToggleSwitch();
    m_ipv6DhcpBox->setChecked(true);
    v6Form->addRow(ipv6DhcpLabel, m_ipv6DhcpBox);

    m_ipv6Edit = new QLineEdit();
    m_ipv6Edit->setPlaceholderText("2001:db8::1");
    m_ipv6ErrLabel = new QLabel();
    m_ipv6ErrLabel->setStyleSheet("color: #e53935; font-size: 8pt; margin-left: 2px;");
    m_ipv6ErrLabel->hide();
    auto* v6ipRow = new QWidget();
    auto* v6ipLay = new QVBoxLayout(v6ipRow);
    v6ipLay->setContentsMargins(0,0,0,0);
    v6ipLay->setSpacing(1);
    v6ipLay->addWidget(m_ipv6Edit);
    v6ipLay->addWidget(m_ipv6ErrLabel);
    v6Form->addRow(new QLabel(tr("IPv6 Address")), v6ipRow);

    m_ipv6PrefEdit = new QLineEdit();
    m_ipv6PrefEdit->setPlaceholderText("64");
    m_ipv6PrefErrLabel = new QLabel();
    m_ipv6PrefErrLabel->setStyleSheet("color: #e53935; font-size: 8pt; margin-left: 2px;");
    m_ipv6PrefErrLabel->hide();
    auto* v6pfRow = new QWidget();
    auto* v6pfLay = new QVBoxLayout(v6pfRow);
    v6pfLay->setContentsMargins(0,0,0,0);
    v6pfLay->setSpacing(1);
    v6pfLay->addWidget(m_ipv6PrefEdit);
    v6pfLay->addWidget(m_ipv6PrefErrLabel);
    v6Form->addRow(new QLabel(tr("Prefix Length")), v6pfRow);

    m_ipv6GwEdit = new QLineEdit();
    m_ipv6GwEdit->setPlaceholderText("fe80::1%12");
    m_ipv6GwErrLabel = new QLabel();
    m_ipv6GwErrLabel->setStyleSheet("color: #e53935; font-size: 8pt; margin-left: 2px;");
    m_ipv6GwErrLabel->hide();
    auto* v6gwRow = new QWidget();
    auto* v6gwLay = new QVBoxLayout(v6gwRow);
    v6gwLay->setContentsMargins(0,0,0,0);
    v6gwLay->setSpacing(1);
    v6gwLay->addWidget(m_ipv6GwEdit);
    v6gwLay->addWidget(m_ipv6GwErrLabel);
    v6Form->addRow(new QLabel(tr("Default Gateway")), v6gwRow);

    m_ipv6DnsBox = new ToggleSwitch();
    m_ipv6DnsBox->setChecked(true);
    v6Form->addRow(new QLabel(tr("Automatic DNS")), m_ipv6DnsBox);

    m_ipv6DnsPrefEdit = new QLineEdit();
    m_ipv6DnsPrefEdit->setPlaceholderText("2001:4860:4860::8888");
    m_ipv6DnsPrefErrLabel = new QLabel();
    m_ipv6DnsPrefErrLabel->setStyleSheet("color: #e53935; font-size: 8pt; margin-left: 2px;");
    m_ipv6DnsPrefErrLabel->hide();
    auto* v6dpfRow = new QWidget();
    auto* v6dpfLay = new QVBoxLayout(v6dpfRow);
    v6dpfLay->setContentsMargins(0,0,0,0);
    v6dpfLay->setSpacing(1);
    v6dpfLay->addWidget(m_ipv6DnsPrefEdit);
    v6dpfLay->addWidget(m_ipv6DnsPrefErrLabel);
    v6Form->addRow(new QLabel(tr("Preferred DNS")), v6dpfRow);

    m_ipv6DnsAltEdit = new QLineEdit();
    m_ipv6DnsAltEdit->setPlaceholderText("2001:4860:4860::8844");
    m_ipv6DnsAltErrLabel = new QLabel();
    m_ipv6DnsAltErrLabel->setStyleSheet("color: #e53935; font-size: 8pt; margin-left: 2px;");
    m_ipv6DnsAltErrLabel->hide();
    auto* v6darRow = new QWidget();
    auto* v6darLay = new QVBoxLayout(v6darRow);
    v6darLay->setContentsMargins(0,0,0,0);
    v6darLay->setSpacing(1);
    v6darLay->addWidget(m_ipv6DnsAltEdit);
    v6darLay->addWidget(m_ipv6DnsAltErrLabel);
    v6Form->addRow(new QLabel(tr("Alternate DNS")), v6darRow);
    tabWidget->addTab(v6Page, tr("IPv6"));

    // Insert tab widget after the name+device rows in rootLayout
    rootLayout->insertWidget(1, tabWidget);

    // IPv6 DHCP toggle
    connect(m_ipv6DhcpBox, &ToggleSwitch::toggled, this, [this](bool on) {
        bool en = !on;
        m_ipv6Edit->setEnabled(en);
        m_ipv6PrefEdit->setEnabled(en);
        m_ipv6GwEdit->setEnabled(en);
        if (on) {
            clearError(m_ipv6Edit, m_ipv6ErrLabel);
            clearError(m_ipv6PrefEdit, m_ipv6PrefErrLabel);
            clearError(m_ipv6GwEdit, m_ipv6GwErrLabel);
        }
    });
    connect(m_ipv6DnsBox, &ToggleSwitch::toggled, this, [this](bool on) {
        bool en = !on;
        m_ipv6DnsPrefEdit->setEnabled(en);
        m_ipv6DnsAltEdit->setEnabled(en);
        if (on) {
            clearError(m_ipv6DnsPrefEdit, m_ipv6DnsPrefErrLabel);
            clearError(m_ipv6DnsAltEdit, m_ipv6DnsAltErrLabel);
        }
    });

    auto installIpv6Validator = [this](QLineEdit* edit, QLabel* errLabel, bool optional) {
        connect(edit, &QLineEdit::textChanged, this, [this, edit, errLabel, optional](const QString& text) {
            if (!edit->isEnabled()) return;
            if (text.trimmed().isEmpty()) {
                if (optional) { clearError(edit, errLabel); return; }
                setError(edit, errLabel, tr("Required"));
                return;
            }
            if (!Profile::isValidIPv6(text.trimmed())) {
                setError(edit, errLabel, tr("Invalid IPv6"));
            } else {
                clearError(edit, errLabel);
            }
        });
    };
    installIpv6Validator(m_ipv6Edit, m_ipv6ErrLabel, false);
    installIpv6Validator(m_ipv6GwEdit, m_ipv6GwErrLabel, true);
    installIpv6Validator(m_ipv6DnsPrefEdit, m_ipv6DnsPrefErrLabel, true);
    installIpv6Validator(m_ipv6DnsAltEdit, m_ipv6DnsAltErrLabel, true);

    connect(m_ipv6PrefEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!m_ipv6PrefEdit->isEnabled()) return;
        if (text.trimmed().isEmpty()) {
            setError(m_ipv6PrefEdit, m_ipv6PrefErrLabel, tr("Required"));
            return;
        }
        bool ok;
        int v = text.trimmed().toInt(&ok);
        if (!ok || v < 0 || v > 128)
            setError(m_ipv6PrefEdit, m_ipv6PrefErrLabel, tr("Must be 0-128"));
        else
            clearError(m_ipv6PrefEdit, m_ipv6PrefErrLabel);
    });

    {
        bool en = !m_ipv6DhcpBox->isChecked();
        m_ipv6Edit->setEnabled(en);
        m_ipv6PrefEdit->setEnabled(en);
        m_ipv6GwEdit->setEnabled(en);
    }
    {
        bool en = !m_ipv6DnsBox->isChecked();
        m_ipv6DnsPrefEdit->setEnabled(en);
        m_ipv6DnsAltEdit->setEnabled(en);
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileEditDialog::onAccept);
}

void ProfileEditDialog::loadFrom(const Profile& p)
{
    m_nameEdit->setText(p.name);
    const NetworkAdapterBinding binding =
        NetworkAdapter::resolve(p.deviceGuid, p.deviceName);
    const QString currentDeviceName =
        binding.isValid() ? binding.name : p.deviceName;
    int idx = m_deviceCombo->findText(currentDeviceName);
    if (idx >= 0) m_deviceCombo->setCurrentIndex(idx);
    else if (!currentDeviceName.isEmpty()) {
        m_deviceCombo->insertItem(0, currentDeviceName);
        m_deviceCombo->setCurrentIndex(0);
    }
    m_dhcpBox->setChecked(p.dhcp);
    m_ipEdit->setText(p.ipAddress);
    m_maskEdit->setText(p.subnetMask);
    m_gwEdit->setText(p.gateway);
    m_defaultDnsBox->setChecked(p.defaultDns);
    m_prefDnsEdit->setText(p.preferredDns);
    m_altDnsEdit->setText(p.alternateDns);

    bool staticIp = !p.dhcp;
    m_ipEdit->setEnabled(staticIp);
    m_maskEdit->setEnabled(staticIp);
    m_gwEdit->setEnabled(staticIp);

    bool customDns = !p.defaultDns;
    m_prefDnsEdit->setEnabled(customDns);
    m_altDnsEdit->setEnabled(customDns);

    m_ipv6DhcpBox->setChecked(p.ipv6Dhcp);
    m_ipv6Edit->setText(p.ipv6Address);
    m_ipv6PrefEdit->setText(p.ipv6Dhcp ? QString() : QString::number(p.ipv6Prefix));
    m_ipv6GwEdit->setText(p.ipv6Gateway);
    m_ipv6DnsBox->setChecked(p.ipv6DefaultDns);
    m_ipv6DnsPrefEdit->setText(p.ipv6PreferredDns);
    m_ipv6DnsAltEdit->setText(p.ipv6AlternateDns);
    bool v6static = !p.ipv6Dhcp;
    m_ipv6Edit->setEnabled(v6static);
    m_ipv6PrefEdit->setEnabled(v6static);
    m_ipv6GwEdit->setEnabled(v6static);
    bool v6dns = !p.ipv6DefaultDns;
    m_ipv6DnsPrefEdit->setEnabled(v6dns);
    m_ipv6DnsAltEdit->setEnabled(v6dns);
}

Profile ProfileEditDialog::profile() const
{
    Profile p;
    p.name          = m_nameEdit->text().trimmed();
    p.deviceName    = m_deviceCombo->currentText();
    const NetworkAdapterBinding binding =
        NetworkAdapter::resolve({}, p.deviceName);
    p.deviceGuid    = binding.isValid()
        ? binding.guid
        : m_originalDeviceGuid;
    p.dhcp          = m_dhcpBox->isChecked();
    p.ipAddress     = m_ipEdit->text().trimmed();
    p.subnetMask    = m_maskEdit->text().trimmed();
    p.gateway       = m_gwEdit->text().trimmed();
    p.defaultDns    = m_defaultDnsBox->isChecked();
    p.preferredDns  = m_prefDnsEdit->text().trimmed();
    p.alternateDns  = m_altDnsEdit->text().trimmed();
    p.ipv6Dhcp      = m_ipv6DhcpBox->isChecked();
    p.ipv6Address   = m_ipv6Edit->text().trimmed();
    p.ipv6Prefix    = m_ipv6PrefEdit->text().trimmed().toInt();
    p.ipv6Gateway   = m_ipv6GwEdit->text().trimmed();
    p.ipv6DefaultDns= m_ipv6DnsBox->isChecked();
    p.ipv6PreferredDns = m_ipv6DnsPrefEdit->text().trimmed();
    p.ipv6AlternateDns = m_ipv6DnsAltEdit->text().trimmed();
    return p;
}

void ProfileEditDialog::prefill(const Profile& p)
{
    const NetworkAdapterBinding binding =
        NetworkAdapter::resolve(p.deviceGuid, p.deviceName);
    const QString currentDeviceName =
        binding.isValid() ? binding.name : p.deviceName;
    m_deviceCombo->setCurrentIndex(m_deviceCombo->findText(currentDeviceName));
    m_dhcpBox->setChecked(p.dhcp);
    m_ipEdit->setText(p.ipAddress);
    m_maskEdit->setText(p.subnetMask);
    m_gwEdit->setText(p.gateway);
    m_defaultDnsBox->setChecked(p.defaultDns);
    m_prefDnsEdit->setText(p.preferredDns);
    m_altDnsEdit->setText(p.alternateDns);

    bool staticIp = !p.dhcp;
    m_ipEdit->setEnabled(staticIp);
    m_maskEdit->setEnabled(staticIp);
    m_gwEdit->setEnabled(staticIp);
    bool customDns = !p.defaultDns;
    m_prefDnsEdit->setEnabled(customDns);
    m_altDnsEdit->setEnabled(customDns);

    m_ipv6DhcpBox->setChecked(p.ipv6Dhcp);
    m_ipv6Edit->setText(p.ipv6Address);
    m_ipv6PrefEdit->setText(p.ipv6Dhcp ? QString() : QString::number(p.ipv6Prefix));
    m_ipv6GwEdit->setText(p.ipv6Gateway);
    m_ipv6DnsBox->setChecked(p.ipv6DefaultDns);
    m_ipv6DnsPrefEdit->setText(p.ipv6PreferredDns);
    m_ipv6DnsAltEdit->setText(p.ipv6AlternateDns);
}

void ProfileEditDialog::setError(QLineEdit* edit, QLabel* errLabel, const QString& msg)
{
    edit->setStyleSheet("border: 2px solid #e53935; border-radius: 3px; padding: 5px 6px;");
    errLabel->setText(msg);
    errLabel->show();
}

void ProfileEditDialog::clearError(QLineEdit* edit, QLabel* errLabel)
{
    edit->setStyleSheet("");
    errLabel->hide();
}

void ProfileEditDialog::onAccept()
{
    Profile p = profile();
    QString reason;
    if (!p.isValid(&reason)) {
        QMessageBox::warning(this, tr("Invalid profile"), reason);
        return;
    }
    if (p.name != m_originalName && m_store.contains(p.name)) {
        QMessageBox::warning(this, tr("Duplicate"),
                             tr("A profile named '%1' already exists.").arg(p.name));
        return;
    }
    accept();
}
