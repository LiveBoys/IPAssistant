#include "ProfileStore.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

ProfileStore::ProfileStore(QObject* parent)
    : QObject(parent)
{
}

int ProfileStore::indexOf(const QString& name) const
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].name == name) return i;
    }
    return -1;
}

bool ProfileStore::contains(const QString& name) const
{
    return indexOf(name) >= 0;
}

bool ProfileStore::upsert(const Profile& p)
{
    QString reason;
    if (!p.isValid(&reason)) {
        qWarning() << "Refusing to upsert invalid profile:" << reason;
        return false;
    }

    int idx = indexOf(p.name);
    if (idx >= 0) {
        m_profiles[idx] = p;
    } else {
        m_profiles.append(p);
    }
    emit changed();
    return true;
}

bool ProfileStore::remove(const QString& name)
{
    int idx = indexOf(name);
    if (idx < 0) return false;
    m_profiles.removeAt(idx);
    emit changed();
    return true;
}

bool ProfileStore::load(const QString& filePath, QString* error)
{
    QFile f(filePath);
    if (!f.exists()) {
        // First run: not an error.
        m_lastFilePath = filePath;
        return true;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = f.errorString();
        return false;
    }

    QByteArray bytes = f.readAll();
    f.close();

    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &perr);
    if (perr.error != QJsonParseError::NoError) {
        if (error) *error = perr.errorString();
        return false;
    }
    if (!doc.isObject()) {
        if (error) *error = "Top-level JSON value must be an object.";
        return false;
    }

    QJsonArray arr = doc.object().value("profiles").toArray();
    QVector<Profile> next;
    next.reserve(arr.size());
    for (const auto& v : arr) {
        Profile p = Profile::fromJson(v.toObject());
        QString reason;
        if (!p.isValid(&reason)) {
            qWarning() << "Skipping invalid profile" << p.name << ":" << reason;
            continue;
        }
        next.append(p);
    }
    m_profiles = std::move(next);
    m_lastFilePath = filePath;
    emit changed();
    return true;
}

bool ProfileStore::save(const QString& filePath, QString* error) const
{
    QJsonArray arr;
    for (const auto& p : m_profiles) {
        arr.append(p.toJson());
    }
    QJsonObject root;
    root["version"]   = 2;
    root["profiles"]  = arr;

    QSaveFile out(filePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = out.errorString();
        return false;
    }
    QJsonDocument doc(root);
    out.write(doc.toJson(QJsonDocument::Indented));
    if (!out.commit()) {
        if (error) *error = out.errorString();
        return false;
    }
    return true;
}
