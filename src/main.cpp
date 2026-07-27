#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

#include "ui/MainWindow.h"
#include "core/ProfileStore.h"
#include "core/NetworkAdapter.h"
#include "core/LocaleManager.h"

namespace {

bool migrateProfileBindings(ProfileStore& store,
                            const QString& filePath,
                            QString* error)
{
    QVector<Profile> migratedProfiles = store.profiles();
    bool changed = false;
    for (auto& profile : migratedProfiles) {
        const NetworkAdapterBinding binding =
            NetworkAdapter::resolve(profile.deviceGuid, profile.deviceName);
        if (!binding.isValid())
            continue;

        if (profile.deviceGuid.compare(binding.guid, Qt::CaseInsensitive) != 0 ||
            profile.deviceName != binding.name) {
            profile.deviceGuid = binding.guid;
            profile.deviceName = binding.name;
            changed = true;
        }
    }
    if (!changed)
        return true;

    const QString backupPath = filePath + QStringLiteral(".bak");
    if (QFileInfo::exists(filePath) && !QFileInfo::exists(backupPath) &&
        !QFile::copy(filePath, backupPath)) {
        if (error) {
            *error = QObject::tr("Failed to create profile backup: %1")
                .arg(backupPath);
        }
        return false;
    }

    for (const auto& profile : migratedProfiles) {
        if (!store.upsert(profile)) {
            if (error) {
                *error = QObject::tr("Failed to migrate profile '%1'.")
                    .arg(profile.name);
            }
            return false;
        }
    }
    return store.save(filePath, error);
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("IP Assistant");
    QApplication::setOrganizationName("IPAssistant");
    QApplication::setApplicationVersion("1.0.0");

    // Init locale (auto-detect or load saved preference).
    LocaleManager::instance().init();

    // Ensure portable data directory exists next to the executable.
    QString dataDir = QCoreApplication::applicationDirPath() + "/data";
    QDir().mkpath(dataDir);

    const QString profileFilePath = dataDir + QStringLiteral("/profiles.json");

    // Load persistent profiles.
    ProfileStore store;
    QString error;
    if (!store.load(profileFilePath, &error)) {
        qWarning() << "Failed to load profiles:" << error;
    }

    // Build a list of available network adapters for the UI.
    QStringList adapters = NetworkAdapter::enumerate();
    if (!migrateProfileBindings(store, profileFilePath, &error)) {
        qWarning() << "Failed to migrate profile adapter bindings:" << error;
    }

    MainWindow window(store, adapters);
    window.show();

    return app.exec();
}
