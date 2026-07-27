#include "Profile.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QJsonObject>
#include <QObject>
#include <QRegularExpression>

QJsonObject Profile::toJson() const
{
    QJsonObject obj;
    obj["name"]          = name;
    obj["deviceName"]    = deviceName;
    obj["deviceGuid"]    = deviceGuid;
    obj["dhcp"]          = dhcp;
    obj["ipAddress"]     = ipAddress;
    obj["subnetMask"]    = subnetMask;
    obj["gateway"]       = gateway;
    obj["defaultDns"]    = defaultDns;
    obj["preferredDns"]  = preferredDns;
    obj["alternateDns"]  = alternateDns;
    obj["ipv6Dhcp"]      = ipv6Dhcp;
    obj["ipv6Address"]   = ipv6Address;
    obj["ipv6Prefix"]    = ipv6Prefix;
    obj["ipv6Gateway"]   = ipv6Gateway;
    obj["ipv6DefaultDns"]= ipv6DefaultDns;
    obj["ipv6Preferred"] = ipv6PreferredDns;
    obj["ipv6Alternate"] = ipv6AlternateDns;
    return obj;
}

Profile Profile::fromJson(const QJsonObject& obj)
{
    Profile p;
    p.name          = obj.value("name").toString();
    p.deviceName    = obj.value("deviceName").toString();
    p.deviceGuid    = obj.value("deviceGuid").toString();
    p.dhcp          = obj.value("dhcp").toBool(true);
    p.ipAddress     = obj.value("ipAddress").toString();
    p.subnetMask    = obj.value("subnetMask").toString();
    p.gateway       = obj.value("gateway").toString();
    p.defaultDns    = obj.value("defaultDns").toBool(false);
    p.preferredDns  = obj.value("preferredDns").toString();
    p.alternateDns  = obj.value("alternateDns").toString();
    p.ipv6Dhcp      = obj.value("ipv6Dhcp").toBool(true);
    p.ipv6Address   = obj.value("ipv6Address").toString();
    p.ipv6Prefix    = obj.value("ipv6Prefix").toInt(64);
    p.ipv6Gateway   = obj.value("ipv6Gateway").toString();
    p.ipv6DefaultDns= obj.value("ipv6DefaultDns").toBool(false);
    p.ipv6PreferredDns = obj.value("ipv6Preferred").toString();
    p.ipv6AlternateDns = obj.value("ipv6Alternate").toString();
    return p;
}

bool Profile::isValidIPv4(const QString& s)
{
    static const QRegularExpression rx(
        QStringLiteral(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)"));
    auto m = rx.match(s);
    if (!m.hasMatch()) return false;
    for (int i = 1; i <= 4; ++i) {
        int octet = m.captured(i).toInt();
        if (octet < 0 || octet > 255) return false;
    }
    return true;
}

bool Profile::isValidIPv6(const QString& s)
{
    if (s.isEmpty()) return false;
    QHostAddress addr(s);
    return !addr.isNull() && addr.protocol() == QAbstractSocket::IPv6Protocol;
}

bool Profile::isValid(QString* reason) const
{
    auto fail = [&](const QString& msg) {
        if (reason) *reason = msg;
        return false;
    };

    if (name.trimmed().isEmpty())
        return fail(QObject::tr("Profile name is required."));
    if (deviceName.trimmed().isEmpty())
        return fail(QObject::tr("Device is required."));

    if (!dhcp) {
        if (!isValidIPv4(ipAddress))
            return fail(QObject::tr("IP address is not a valid IPv4."));
        if (!isValidIPv4(subnetMask))
            return fail(QObject::tr("Subnet mask is not a valid IPv4."));
        if (!gateway.isEmpty() && !isValidIPv4(gateway))
            return fail(QObject::tr("Default gateway is not a valid IPv4."));
    }

    if (!defaultDns) {
        if (!preferredDns.isEmpty() && !isValidIPv4(preferredDns))
            return fail(QObject::tr("Preferred DNS is not a valid IPv4."));
        if (!alternateDns.isEmpty() && !isValidIPv4(alternateDns))
            return fail(QObject::tr("Alternate DNS is not a valid IPv4."));
    }

    // IPv6 validation
    if (!ipv6Dhcp) {
        if (!isValidIPv6(ipv6Address))
            return fail(QObject::tr("IPv6 address is not a valid IPv6."));
        if (ipv6Prefix < 0 || ipv6Prefix > 128)
            return fail(QObject::tr("IPv6 prefix must be 0-128."));
        if (!ipv6Gateway.isEmpty() && !isValidIPv6(ipv6Gateway))
            return fail(QObject::tr("IPv6 gateway is not a valid IPv6."));
    }

    if (!ipv6DefaultDns) {
        if (!ipv6PreferredDns.isEmpty() && !isValidIPv6(ipv6PreferredDns))
            return fail(QObject::tr("IPv6 preferred DNS is not a valid IPv6."));
        if (!ipv6AlternateDns.isEmpty() && !isValidIPv6(ipv6AlternateDns))
            return fail(QObject::tr("IPv6 alternate DNS is not a valid IPv6."));
    }

    return true;
}
