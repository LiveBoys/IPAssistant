#include "NetworkAdapter.h"

#include <QNetworkInterface>
#include <QHostAddress>

#ifdef Q_OS_WIN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <iphlpapi.h>
#  include <windows.h>
#endif

QStringList NetworkAdapter::enumerate()
{
    QStringList result;

#ifdef Q_OS_WIN
    ULONG bufLen = 16 * 1024;
    QByteArray buffer;
    buffer.resize(static_cast<int>(bufLen));

    DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG iterations = 0;
    DWORD rc = ERROR_BUFFER_OVERFLOW;
    while (rc == ERROR_BUFFER_OVERFLOW && iterations++ < 4) {
        rc = GetAdaptersAddresses(AF_INET, flags, nullptr,
                                  reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
                                  &bufLen);
        if (rc == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(static_cast<int>(bufLen));
        }
    }
    if (rc != NO_ERROR) {
        QStringList fallback;
        const auto all = QNetworkInterface::allInterfaces();
        for (const auto& iface : all) {
            if (!(iface.flags() & QNetworkInterface::IsLoopBack)) {
                fallback.append(iface.humanReadableName());
            }
        }
        fallback.sort();
        return fallback;
    }

    auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    for (; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp &&
            adapter->OperStatus != IfOperStatusUnknown) {
            continue;
        }
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        QString friendly;
        if (adapter->FriendlyName) {
            friendly = QString::fromWCharArray(adapter->FriendlyName);
        }
        if (!friendly.isEmpty() && !result.contains(friendly)) {
            result.append(friendly);
        }
    }
#endif

    if (result.isEmpty()) {
        const auto all = QNetworkInterface::allInterfaces();
        for (const auto& iface : all) {
            if (!(iface.flags() & QNetworkInterface::IsLoopBack)) {
                result.append(iface.humanReadableName());
            }
        }
    }

    result.sort();
    return result;
}

QString NetworkAdapter::interfaceNameFor(const QString& friendlyName)
{
#ifdef Q_OS_WIN
    return resolve({}, friendlyName).guid;
#else
    return friendlyName;
#endif
}

QString NetworkAdapter::interfaceGuidFor(const QString& friendlyName)
{
#ifdef Q_OS_WIN
    return resolve({}, friendlyName).guid;
#else
    return {};
#endif
}

NetworkAdapterBinding NetworkAdapter::resolve(const QString& guid,
                                              const QString& fallbackName)
{
#ifdef Q_OS_WIN
    ULONG bufferLength = 16 * 1024;
    QByteArray buffer(static_cast<int>(bufferLength), '\0');
    const DWORD flags =
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG iterations = 0;
    DWORD status = ERROR_BUFFER_OVERFLOW;
    while (status == ERROR_BUFFER_OVERFLOW && iterations++ < 4) {
        status = GetAdaptersAddresses(
            AF_UNSPEC,
            flags,
            nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
            &bufferLength);
        if (status == ERROR_BUFFER_OVERFLOW)
            buffer.resize(static_cast<int>(bufferLength));
    }
    if (status != NO_ERROR)
        return {};

    NetworkAdapterBinding nameFallback;
    for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
         adapter;
         adapter = adapter->Next) {
        if (!adapter->AdapterName || !adapter->FriendlyName)
            continue;

        NetworkAdapterBinding candidate;
        candidate.name = QString::fromWCharArray(adapter->FriendlyName);
        candidate.guid = QString::fromLocal8Bit(adapter->AdapterName);

        if (!guid.isEmpty() &&
            candidate.guid.compare(guid, Qt::CaseInsensitive) == 0) {
            return candidate;
        }
        if (nameFallback.name.isEmpty() &&
            !fallbackName.isEmpty() &&
            candidate.name.compare(fallbackName, Qt::CaseInsensitive) == 0) {
            nameFallback = candidate;
        }
    }
    return nameFallback;
#else
    const auto adapters = enumerate();
    for (const auto& name : adapters) {
        if (name.compare(fallbackName, Qt::CaseInsensitive) == 0)
            return { name, guid };
    }
    return {};
#endif
}

QString NetworkAdapter::friendlyNameForGuid(const QString& guid)
{
    return resolve(guid).name;
}

#ifdef Q_OS_WIN

/// Query the default gateway for a given interface index via GetIpForwardTable.
static QString queryDefaultGateway(IF_INDEX ifIndex)
{
    PMIB_IPFORWARDTABLE table = nullptr;
    ULONG size = 0;
    DWORD rc = GetIpForwardTable(table, &size, FALSE);
    if (rc != ERROR_INSUFFICIENT_BUFFER || size == 0) return {};

    table = reinterpret_cast<MIB_IPFORWARDTABLE*>(malloc(size));
    if (!table) return {};

    rc = GetIpForwardTable(table, &size, FALSE);
    if (rc != NO_ERROR) {
        free(table);
        return {};
    }

    QString gateway;
    DWORD bestMetric = 0xFFFFFFFF;

    for (DWORD i = 0; i < table->dwNumEntries; ++i) {
        const auto& row = table->table[i];
        // Match interface index and destination 0.0.0.0 (default route).
        if (row.dwForwardIfIndex != ifIndex) continue;
        if (row.dwForwardDest != 0) continue;

        if (row.dwForwardMetric1 < bestMetric) {
            bestMetric = row.dwForwardMetric1;
            gateway = QHostAddress(ntohl(row.dwForwardNextHop)).toString();
        }
    }

    free(table);
    return gateway;
}

#endif

Profile NetworkAdapter::queryCurrentConfig(const QString& friendlyName)
{
    Profile p;
    p.deviceName = friendlyName;
    p.deviceGuid = interfaceGuidFor(friendlyName);

#ifdef Q_OS_WIN
    // --- Step 1: Get adapter addresses (IP, prefix, gateway, DNS, DHCP) ---
    ULONG bufLen = 16 * 1024;
    QByteArray buffer;
    buffer.resize(static_cast<int>(bufLen));

    DWORD flags = GAA_FLAG_INCLUDE_PREFIX
                | GAA_FLAG_SKIP_ANYCAST
                | GAA_FLAG_SKIP_MULTICAST;
    ULONG iterations = 0;
    DWORD rc = ERROR_BUFFER_OVERFLOW;
    while (rc == ERROR_BUFFER_OVERFLOW && iterations++ < 4) {
        rc = GetAdaptersAddresses(AF_INET, flags, nullptr,
                                  reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
                                  &bufLen);
        if (rc == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(static_cast<int>(bufLen));
        }
    }
    if (rc != NO_ERROR) return p;

    auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    for (; adapter != nullptr; adapter = adapter->Next) {
        QString f;
        if (adapter->FriendlyName)
            f = QString::fromWCharArray(adapter->FriendlyName);
        if (f != friendlyName) continue;

        // --- DHCP ---
        p.dhcp = (adapter->Flags & IP_ADAPTER_DHCP_ENABLED) != 0;

        // --- IP + mask (from FirstUnicastAddress) ---
        for (auto* addr = adapter->FirstUnicastAddress; addr; addr = addr->Next) {
            if (addr->Address.lpSockaddr->sa_family != AF_INET) continue;
            auto* sin = reinterpret_cast<sockaddr_in*>(addr->Address.lpSockaddr);
            p.ipAddress = QHostAddress(ntohl(sin->sin_addr.s_addr)).toString();

            // Convert prefix length (0-32) to dotted mask.
            // QHostAddress(quint32) expects host byte order.
            quint8 prefix = addr->OnLinkPrefixLength;
            if (prefix > 0 && prefix <= 32) {
                quint32 maskHost = (prefix == 32) ? 0xFFFFFFFFu
                                                  : (0xFFFFFFFFu << (32 - prefix));
                p.subnetMask = QHostAddress(maskHost).toString();
            } else {
                p.subnetMask.clear();
            }
            break;  // first IPv4 only
        }

        // If prefix length didn't work, try IP_ADAPTER_PREFIX list.
        if (p.subnetMask.isEmpty()) {
            for (auto* pref = adapter->FirstPrefix; pref; pref = pref->Next) {
                if (pref->Address.lpSockaddr->sa_family != AF_INET) continue;
                quint8 prefix = pref->PrefixLength;
                if (prefix > 0 && prefix <= 32) {
                    quint32 maskHost = (prefix == 32) ? 0xFFFFFFFFu
                                                      : (0xFFFFFFFFu << (32 - prefix));
                    p.subnetMask = QHostAddress(maskHost).toString();
                    break;
                }
            }
        }

        // --- Gateway (from FirstGatewayAddress) ---
        for (auto* gw = adapter->FirstGatewayAddress; gw; gw = gw->Next) {
            if (gw->Address.lpSockaddr->sa_family != AF_INET) continue;
            auto* sin = reinterpret_cast<sockaddr_in*>(gw->Address.lpSockaddr);
            p.gateway = QHostAddress(ntohl(sin->sin_addr.s_addr)).toString();
            break;
        }

        // --- Gateway fallback: query routing table for default gateway ---
        if (p.gateway.isEmpty()) {
            p.gateway = queryDefaultGateway(adapter->IfIndex);
        }

        // --- DNS ---
        for (auto* dns = adapter->FirstDnsServerAddress; dns; dns = dns->Next) {
            if (dns->Address.lpSockaddr->sa_family != AF_INET) continue;
            auto* sin = reinterpret_cast<sockaddr_in*>(dns->Address.lpSockaddr);
            QString ip = QHostAddress(ntohl(sin->sin_addr.s_addr)).toString();
            if (p.preferredDns.isEmpty()) {
                p.preferredDns = ip;
            } else if (p.alternateDns.isEmpty()) {
                p.alternateDns = ip;
                break;
            }
        }
        p.defaultDns = p.preferredDns.isEmpty();

        break;  // found our adapter
    }

#else
    // Fallback for non-Windows.
    const auto all = QNetworkInterface::allInterfaces();
    for (const auto& iface : all) {
        if (iface.humanReadableName() != friendlyName) continue;
        const auto entries = iface.addressEntries();
        for (const auto& entry : entries) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            p.ipAddress  = entry.ip().toString();
            p.subnetMask = entry.netmask().toString();
            break;
        }
        break;
    }
    p.dhcp = true;
    p.defaultDns = true;
#endif

    return p;
}
