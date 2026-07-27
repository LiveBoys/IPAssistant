#ifndef IPSWITCH_LOCALE_MANAGER_H
#define IPSWITCH_LOCALE_MANAGER_H

#include <QObject>
#include <QList>
#include <QTranslator>

/// Global language manager: installs/removes translators on the fly.
class LocaleManager : public QObject
{
    Q_OBJECT
public:
    struct Language
    {
        QString code;
        QString nativeName;
    };

    static LocaleManager& instance();

    /// Languages bundled with the application, in UI display order.
    static const QList<Language>& supportedLanguages();

    /// Convert a system/user locale to one of the bundled language codes.
    static QString normalizedLanguage(const QString& localeName);

    /// Switch UI language immediately. Unsupported values fall back to en_US.
    void switchLanguage(const QString& lang);

    /// Current language code.
    QString currentLanguage() const { return m_currentLang; }

    /// Initialize on app startup (auto-detect or load saved preference).
    void init();

signals:
    /// Emitted after switchLanguage() completes, so all windows can retranslate.
    void languageChanged(const QString& lang);

private:
    LocaleManager() = default;
    QTranslator m_translator;
    QTranslator m_qtTranslator;
    QString     m_currentLang;
};

#endif // IPSWITCH_LOCALE_MANAGER_H
