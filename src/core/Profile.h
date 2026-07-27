#ifndef IPSWITCH_PROFILE_H
#define IPSWITCH_PROFILE_H

#include <QString>
#include <QStringList>
#include <QJsonObject>

/**
 * @brief A single IP configuration profile bound to a network adapter.
 *
 * Each profile describes IPv4 and IPv6 settings. A profile is "active"
 * when it has been applied on the underlying OS.
 */
struct Profile
{
    QString name;              ///< Display name (unique within store).
    QString deviceName;       ///< Adapter friendly name used for display/fallback.
    QString deviceGuid;       ///< Stable Windows adapter GUID used for binding.
    bool    dhcp = true;       ///< IPv4 DHCP.
    QString ipAddress;         ///< Static IPv4 address.
    QString subnetMask;        ///< Static IPv4 subnet mask.
    QString gateway;           ///< Static IPv4 gateway (may be empty).
    bool    defaultDns = false;
    QString preferredDns;
    QString alternateDns;

    // --- IPv6 ---
    bool    ipv6Dhcp = true;   ///< IPv6 auto-configuration (DHCPv6 / SLAAC).
    QString ipv6Address;       ///< Static IPv6 address.
    int     ipv6Prefix = 64;   ///< Prefix length (0-128, default 64).
    QString ipv6Gateway;
    bool    ipv6DefaultDns = false;
    QString ipv6PreferredDns;
    QString ipv6AlternateDns;

    QJsonObject toJson() const;
    static Profile fromJson(const QJsonObject& obj);
    bool isValid(QString* reason = nullptr) const;

    /// Check whether a string is a valid dotted-decimal IPv4 address.
    static bool isValidIPv4(const QString& s);
    /// Check whether a string is a valid IPv6 address.
    static bool isValidIPv6(const QString& s);
};

#endif // IPSWITCH_PROFILE_H
