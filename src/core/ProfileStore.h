#ifndef IPSWITCH_PROFILE_STORE_H
#define IPSWITCH_PROFILE_STORE_H

#include "Profile.h"

#include <QObject>
#include <QVector>
#include <QString>

/**
 * @brief In-memory profile repository with JSON on-disk persistence.
 *
 * Thread affinity: this object must live on the GUI thread. All mutation
 * is O(n) where n = profile count; we don't expect more than ~100 entries.
 */
class ProfileStore : public QObject
{
    Q_OBJECT
public:
    explicit ProfileStore(QObject* parent = nullptr);

    const QVector<Profile>& profiles() const { return m_profiles; }

    int indexOf(const QString& name) const;
    bool contains(const QString& name) const;

    /// Append or replace a profile by name. Returns true on success.
    bool upsert(const Profile& p);

    /// Remove a profile by name. Returns true if something was removed.
    bool remove(const QString& name);

    /// Load profiles from a JSON file. On failure, leaves state unchanged.
    bool load(const QString& filePath, QString* error = nullptr);

    /// Atomically write profiles to a JSON file.
    bool save(const QString& filePath, QString* error = nullptr) const;

signals:
    void changed();

private:
    QVector<Profile> m_profiles;
    QString          m_lastFilePath;
};

#endif // IPSWITCH_PROFILE_STORE_H