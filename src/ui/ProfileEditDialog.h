#ifndef IPSWITCH_PROFILE_EDIT_DIALOG_H
#define IPSWITCH_PROFILE_EDIT_DIALOG_H

#include "core/Profile.h"
#include "ui_ProfileEditDialog.h"

#include <QDialog>
#include <QStringList>

class ProfileStore;

/// Modal dialog used to create or edit a profile.
/// Includes real-time IPv4 validation with inline error messages.
class ProfileEditDialog : public QDialog, private Ui::ProfileEditDialog
{
    Q_OBJECT
public:
    explicit ProfileEditDialog(ProfileStore& store,
                               const QStringList& adapters,
                               QWidget* parent = nullptr);

    explicit ProfileEditDialog(ProfileStore& store,
                               const QStringList& adapters,
                               const Profile& existing,
                               QWidget* parent = nullptr);

    Profile profile() const;
    void prefill(const Profile& p);

private slots:
    void onAccept();

private:
    void init();
    void loadFrom(const Profile& p);
    void setError(QLineEdit* edit, QLabel* errLabel, const QString& msg);
    void clearError(QLineEdit* edit, QLabel* errLabel);

    ProfileStore& m_store;
    QStringList    m_adapters;
    QString        m_originalName;
    QString        m_originalDeviceGuid;

    // IPv6 fields (programmatic)
    ToggleSwitch* m_ipv6DhcpBox = nullptr;
    QLineEdit*    m_ipv6Edit = nullptr;
    QLabel*       m_ipv6ErrLabel = nullptr;
    QLineEdit*    m_ipv6PrefEdit = nullptr;
    QLabel*       m_ipv6PrefErrLabel = nullptr;
    QLineEdit*    m_ipv6GwEdit = nullptr;
    QLabel*       m_ipv6GwErrLabel = nullptr;
    ToggleSwitch* m_ipv6DnsBox = nullptr;
    QLineEdit*    m_ipv6DnsPrefEdit = nullptr;
    QLabel*       m_ipv6DnsPrefErrLabel = nullptr;
    QLineEdit*    m_ipv6DnsAltEdit = nullptr;
    QLabel*       m_ipv6DnsAltErrLabel = nullptr;
};

#endif // IPSWITCH_PROFILE_EDIT_DIALOG_H
