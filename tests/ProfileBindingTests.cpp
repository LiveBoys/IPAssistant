#include "core/NetworkAdapter.h"
#include "core/LocaleManager.h"
#include "core/Profile.h"

#include <QApplication>
#include <QJsonObject>
#include <QtTest>

class ProfileBindingTests : public QObject
{
    Q_OBJECT

private slots:
    void normalizesSupportedLocales();
    void fallsBackForUnsupportedLocales();
    void loadsEveryBundledTranslation();
    void profileGuidRoundTrip();
    void legacyProfileWithoutGuidRemainsValid();
    void guidResolutionFallsBackToName();
};

void ProfileBindingTests::normalizesSupportedLocales()
{
    QCOMPARE(LocaleManager::normalizedLanguage(QStringLiteral("de-DE")),
             QStringLiteral("de_DE"));
    QCOMPARE(LocaleManager::normalizedLanguage(QStringLiteral("zh-Hant-HK")),
             QStringLiteral("zh_TW"));
    QCOMPARE(LocaleManager::normalizedLanguage(QStringLiteral("pt-PT")),
             QStringLiteral("pt_BR"));
    QCOMPARE(LocaleManager::supportedLanguages().size(), 18);
}

void ProfileBindingTests::fallsBackForUnsupportedLocales()
{
    QCOMPARE(LocaleManager::normalizedLanguage(QStringLiteral("nl_NL")),
             QStringLiteral("en_US"));
    QCOMPARE(LocaleManager::normalizedLanguage(QString()),
             QStringLiteral("en_US"));
}

void ProfileBindingTests::loadsEveryBundledTranslation()
{
    LocaleManager& manager = LocaleManager::instance();
    for (const LocaleManager::Language& language :
         LocaleManager::supportedLanguages()) {
        manager.switchLanguage(language.code);
        QCOMPARE(manager.currentLanguage(), language.code);

        const QString translated =
            QCoreApplication::translate("SettingsDialog", "Settings");
        if (language.code == QStringLiteral("en_US")) {
            QCOMPARE(translated, QStringLiteral("Settings"));
        } else {
            QVERIFY2(translated != QStringLiteral("Settings"),
                     qPrintable(QStringLiteral("Translation did not load: %1")
                                    .arg(language.code)));
        }
    }

    manager.switchLanguage(QStringLiteral("ar_SA"));
    QCOMPARE(qApp->layoutDirection(), Qt::RightToLeft);
    manager.switchLanguage(QStringLiteral("en_US"));
    QCOMPARE(qApp->layoutDirection(), Qt::LeftToRight);
}

void ProfileBindingTests::profileGuidRoundTrip()
{
    Profile original;
    original.name = QStringLiteral("Office");
    original.deviceName = QStringLiteral("Ethernet");
    original.deviceGuid = QStringLiteral("{11111111-2222-3333-4444-555555555555}");
    original.dhcp = true;
    original.defaultDns = true;
    original.ipv6Dhcp = true;
    original.ipv6DefaultDns = true;

    const Profile restored = Profile::fromJson(original.toJson());

    QCOMPARE(restored.deviceName, original.deviceName);
    QCOMPARE(restored.deviceGuid, original.deviceGuid);
    QVERIFY(restored.isValid());
}

void ProfileBindingTests::legacyProfileWithoutGuidRemainsValid()
{
    QJsonObject legacy;
    legacy["name"] = QStringLiteral("Legacy");
    legacy["deviceName"] = QStringLiteral("Ethernet");
    legacy["dhcp"] = true;
    legacy["defaultDns"] = true;
    legacy["ipv6Dhcp"] = true;
    legacy["ipv6DefaultDns"] = true;

    const Profile profile = Profile::fromJson(legacy);

    QVERIFY(profile.deviceGuid.isEmpty());
    QVERIFY(profile.isValid());
}

void ProfileBindingTests::guidResolutionFallsBackToName()
{
    const QStringList adapters = NetworkAdapter::enumerate();
    if (adapters.isEmpty())
        QSKIP("No network adapter is available on this host.");

    const QString adapterName = adapters.first();
    const QString adapterGuid = NetworkAdapter::interfaceGuidFor(adapterName);
    QVERIFY2(!adapterGuid.isEmpty(), "The selected adapter has no stable GUID.");

    const NetworkAdapterBinding guidMatch =
        NetworkAdapter::resolve(adapterGuid, QStringLiteral("__wrong_name__"));
    QVERIFY(guidMatch.isValid());
    QCOMPARE(guidMatch.guid.compare(adapterGuid, Qt::CaseInsensitive), 0);

    const NetworkAdapterBinding nameFallback = NetworkAdapter::resolve(
        QStringLiteral("{00000000-0000-0000-0000-000000000000}"),
        adapterName);
    QVERIFY(nameFallback.isValid());
    QCOMPARE(nameFallback.guid.compare(adapterGuid, Qt::CaseInsensitive), 0);

    const NetworkAdapterBinding missing = NetworkAdapter::resolve(
        QStringLiteral("{00000000-0000-0000-0000-000000000000}"),
        QStringLiteral("__missing_adapter__"));
    QVERIFY(!missing.isValid());
}

QTEST_MAIN(ProfileBindingTests)

#include "ProfileBindingTests.moc"
