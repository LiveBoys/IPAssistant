#include "SettingsDialog.h"
#include "ToggleSwitch.h"
#include "core/AutoStart.h"
#include "core/LocaleManager.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    // Native language names remain understandable even if the current locale is wrong.
    for (const LocaleManager::Language& language :
         LocaleManager::supportedLanguages()) {
        m_languageCombo->addItem(language.nativeName, language.code);
    }
    QString savedLang = LocaleManager::instance().currentLanguage();
    if (savedLang.isEmpty()) savedLang = "en_US";
    int idx = m_languageCombo->findData(savedLang);
    if (idx >= 0) m_languageCombo->setCurrentIndex(idx);

    // Load existing values
    QSettings qs;
    m_startWithWindows->setChecked(
        qs.value("ui/startWithWindows", AutoStart::isEnabled()).toBool());
    m_minimizeToTray->setChecked(
        qs.value("ui/minimizeToTray", true).toBool());

    updateStatusLabel();

    // Update status when toggles change
    connect(m_startWithWindows, &ToggleSwitch::toggled, this, &SettingsDialog::updateStatusLabel);
    connect(m_minimizeToTray,   &ToggleSwitch::toggled, this, &SettingsDialog::updateStatusLabel);

    // Buttons
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        AutoStart::setEnabled(m_startWithWindows->isChecked());
        QString lang = m_languageCombo->currentData().toString();
        QSettings qs2;
        qs2.setValue("ui/language", lang);
        LocaleManager::instance().switchLanguage(lang);
        accept();
    });
}

void SettingsDialog::updateStatusLabel()
{
    QStringList parts;
    if (m_startWithWindows->isChecked()) {
        parts << tr("App will start automatically on login.");
    } else {
        parts << tr("App will NOT start on login.");
    }
    if (m_minimizeToTray->isChecked()) {
        parts << tr("Closing the window will hide it to tray.");
    } else {
        parts << tr("Closing the window will exit the application.");
    }
    m_statusLabel->setText(parts.join("\n"));
}

bool SettingsDialog::startWithWindows() const { return m_startWithWindows->isChecked(); }
bool SettingsDialog::minimizeToTray()  const { return m_minimizeToTray->isChecked(); }
