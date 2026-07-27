#include "LocaleManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QLoggingCategory>
#include <QSettings>

namespace {
// Keep standard dialog buttons localized even when Qt does not ship a catalog
// for a supported application language (for example, Hindi or Vietnamese).
[[maybe_unused]] constexpr const char* kStandardButtonTranslations[] = {
    QT_TRANSLATE_NOOP("QPlatformTheme", "OK"),
    QT_TRANSLATE_NOOP("QPlatformTheme", "Save"),
    QT_TRANSLATE_NOOP("QPlatformTheme", "Cancel"),
    QT_TRANSLATE_NOOP("QDialogButtonBox", "OK"),
};
}

LocaleManager& LocaleManager::instance()
{
    static LocaleManager mgr;
    return mgr;
}

const QList<LocaleManager::Language>& LocaleManager::supportedLanguages()
{
    static const QList<Language> languages = {
        {QStringLiteral("en_US"), QStringLiteral("English")},
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
        {QStringLiteral("zh_TW"), QStringLiteral("繁體中文")},
        {QStringLiteral("ja_JP"), QStringLiteral("日本語")},
        {QStringLiteral("ko_KR"), QStringLiteral("한국어")},
        {QStringLiteral("de_DE"), QStringLiteral("Deutsch")},
        {QStringLiteral("fr_FR"), QStringLiteral("Français")},
        {QStringLiteral("es_ES"), QStringLiteral("Español")},
        {QStringLiteral("pt_BR"), QStringLiteral("Português (Brasil)")},
        {QStringLiteral("ru_RU"), QStringLiteral("Русский")},
        {QStringLiteral("it_IT"), QStringLiteral("Italiano")},
        {QStringLiteral("tr_TR"), QStringLiteral("Türkçe")},
        {QStringLiteral("pl_PL"), QStringLiteral("Polski")},
        {QStringLiteral("id_ID"), QStringLiteral("Bahasa Indonesia")},
        {QStringLiteral("vi_VN"), QStringLiteral("Tiếng Việt")},
        {QStringLiteral("th_TH"), QStringLiteral("ไทย")},
        {QStringLiteral("ar_SA"), QStringLiteral("العربية")},
        {QStringLiteral("hi_IN"), QStringLiteral("हिन्दी")},
    };
    return languages;
}

QString LocaleManager::normalizedLanguage(const QString& localeName)
{
    QString normalized = localeName;
    normalized.replace('-', '_');
    for (const Language& language : supportedLanguages()) {
        if (normalized.compare(language.code, Qt::CaseInsensitive) == 0)
            return language.code;
    }

    const QString lower = normalized.toLower();
    if (lower.startsWith(QStringLiteral("zh_hant")) ||
        lower.startsWith(QStringLiteral("zh_tw")) ||
        lower.startsWith(QStringLiteral("zh_hk")) ||
        lower.startsWith(QStringLiteral("zh_mo"))) {
        return QStringLiteral("zh_TW");
    }
    if (lower.startsWith(QStringLiteral("zh")))
        return QStringLiteral("zh_CN");

    for (const Language& language : supportedLanguages()) {
        if (lower.startsWith(language.code.left(2).toLower()))
            return language.code;
    }
    return QStringLiteral("en_US");
}

void LocaleManager::init()
{
    QSettings qs;
    QString lang = qs.value("ui/language").toString();
    if (lang.isEmpty())
        lang = QLocale::system().name();
    switchLanguage(lang);
}

void LocaleManager::switchLanguage(const QString& lang)
{
    const QString targetLang = normalizedLanguage(lang);
    if (m_currentLang == targetLang)
        return;

    // Remove existing translators if any.
    if (!m_currentLang.isEmpty()) {
        qApp->removeTranslator(&m_translator);
        qApp->removeTranslator(&m_qtTranslator);
    }

    m_currentLang = targetLang;
    qApp->setLayoutDirection(
        QLocale(targetLang).textDirection() == Qt::RightToLeft
            ? Qt::RightToLeft
            : Qt::LeftToRight);

    if (targetLang != QStringLiteral("en_US")) {
        // Load app translation.
        const QString translationBase =
            QStringLiteral("ipassistant_") + targetLang;
        if (m_translator.load(QStringLiteral(":/i18n/") + translationBase +
                                  QStringLiteral(".qm")) ||
            m_translator.load(translationBase,
                              QCoreApplication::applicationDirPath() +
                                  QStringLiteral("/translations"))) {
            qApp->installTranslator(&m_translator);
        } else {
            qWarning() << "Application translation could not be loaded:"
                       << targetLang;
        }

        // Load Qt built-in translation for standard button texts etc.
        const QString qtTrDir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        if (m_qtTranslator.load(QStringLiteral("qt_") + targetLang, qtTrDir) ||
            m_qtTranslator.load(QStringLiteral("qt_") + targetLang.left(2),
                                qtTrDir)) {
            qApp->installTranslator(&m_qtTranslator);
        }
    }
    // en_US: no translator needed (source language).

    emit languageChanged(targetLang);
}
