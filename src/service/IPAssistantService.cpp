// Windows headers MUST come before any Qt headers because
// Qt includes may pull in windows.h before winsock2.h.
#define _WIN32_WINNT 0x0A00
#include <winsock2.h>
#include <windows.h>

#include "IPAssistantService.h"
#include "core/Profile.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QObject>
#include <QHostAddress>
#include <QProcess>
#include <QRegularExpression>
#include <QVector>
#include <sddl.h>
#include <netioapi.h>   // Must come BEFORE iphlpapi.h (conditional type definitions)
#include <iphlpapi.h>

static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\IPAssistantService";

// ------------------------------------------------------------------
//  Free functions for Windows Service callbacks
// ------------------------------------------------------------------
namespace {
IPAssistantService* g_serviceSelf = nullptr;

void WINAPI serviceMainImpl(DWORD argc, LPWSTR* argv)
{
    if (g_serviceSelf) g_serviceSelf->onStart(argc, argv);
}

DWORD WINAPI ctrlHandlerImpl(DWORD ctrl, DWORD, LPVOID, LPVOID)
{
    if (ctrl == SERVICE_CONTROL_STOP || ctrl == SERVICE_CONTROL_SHUTDOWN) {
        if (g_serviceSelf) g_serviceSelf->onStop();
    }
    return NO_ERROR;
}
}

// ======================================================================
//  Adapter helpers — get interface index / GUID from friendly name
// ======================================================================
struct AdapterInfo {
    IF_INDEX ifIndex = 0;
    NET_LUID luid    = {};
    QString guid;    // {GUID} string for registry DNS
    QString friendlyName;
    bool valid = false;
};

static AdapterInfo adapterInfoFromAddresses(const IP_ADAPTER_ADDRESSES* adapter)
{
    AdapterInfo info;
    if (!adapter || !adapter->AdapterName || !adapter->FriendlyName)
        return info;

    info.ifIndex = adapter->IfIndex != 0
        ? adapter->IfIndex
        : adapter->Ipv6IfIndex;
    info.luid = adapter->Luid;
    info.guid = QString::fromLocal8Bit(adapter->AdapterName);
    info.friendlyName = QString::fromWCharArray(adapter->FriendlyName);
    info.valid = info.ifIndex != 0 && !info.guid.isEmpty();
    return info;
}

static AdapterInfo resolveAdapter(const QString& guid,
                                  const QString& fallbackFriendlyName)
{
    ULONG bufLen = 16 * 1024;
    QByteArray buf(bufLen, '\0');

    DWORD rc = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
        nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()), &bufLen);
    if (rc == ERROR_BUFFER_OVERFLOW) {
        buf.resize(static_cast<int>(bufLen));
        rc = GetAdaptersAddresses(AF_UNSPEC,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()), &bufLen);
    }
    if (rc != NO_ERROR)
        return {};

    AdapterInfo nameFallback;
    for (auto* a = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()); a; a = a->Next) {
        const AdapterInfo candidate = adapterInfoFromAddresses(a);
        if (!candidate.valid)
            continue;
        if (!guid.isEmpty() &&
            candidate.guid.compare(guid, Qt::CaseInsensitive) == 0) {
            return candidate;
        }
        if (!nameFallback.valid &&
            !fallbackFriendlyName.isEmpty() &&
            candidate.friendlyName.compare(
                fallbackFriendlyName, Qt::CaseInsensitive) == 0) {
            nameFallback = candidate;
        }
    }
    return nameFallback;
}

// ======================================================================
//  Transactional network configuration helpers
// ======================================================================
struct OperationResult {
    bool ok = false;
    int code = -1;
    QString message;

    static OperationResult success()
    {
        OperationResult result;
        result.ok = true;
        result.code = 0;
        return result;
    }

    static OperationResult failure(int errorCode, const QString& errorMessage)
    {
        OperationResult result;
        result.code = errorCode;
        result.message = errorMessage;
        return result;
    }
};

struct DnsConfig {
    bool automatic = true;
    QStringList servers;
};

struct Ipv4Config {
    bool dhcp = true;
    QString address;
    QString subnetMask;
    QString gateway;
    DnsConfig dns;
};

struct NetworkSnapshot {
    Ipv4Config ipv4;
    DnsConfig ipv6Dns;
    QVector<MIB_UNICASTIPADDRESS_ROW> manualIpv6Addresses;
    QVector<MIB_IPFORWARD_ROW2> manualIpv6DefaultRoutes;
};

static QString systemExecutable(const QString& executable)
{
    QString windowsDir = qEnvironmentVariable("SystemRoot");
    if (windowsDir.isEmpty())
        windowsDir = QStringLiteral("C:\\Windows");
    return QDir::toNativeSeparators(windowsDir + QStringLiteral("/System32/") + executable);
}

static OperationResult runNetsh(const QStringList& arguments)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(systemExecutable(QStringLiteral("netsh.exe")), arguments);

    if (!process.waitForStarted(5000)) {
        return OperationResult::failure(
            static_cast<int>(process.error()),
            QObject::tr("Failed to start netsh: %1").arg(process.errorString()));
    }

    if (!process.waitForFinished(20000)) {
        process.kill();
        process.waitForFinished(3000);
        return OperationResult::failure(
            WAIT_TIMEOUT,
            QObject::tr("Network configuration command timed out."));
    }

    const QString stdoutText =
        QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    const QString stderrText =
        QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
    const int exitCode = process.exitCode();
    if (process.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        QString detail = stderrText.isEmpty() ? stdoutText : stderrText;
        if (detail.isEmpty())
            detail = QObject::tr("netsh exited with code %1.").arg(exitCode);
        return OperationResult::failure(exitCode, detail);
    }

    return OperationResult::success();
}

static QString prefixToMask(ULONG prefixLength)
{
    if (prefixLength > 32)
        return {};
    const quint32 mask = prefixLength == 0
        ? 0
        : 0xFFFFFFFFu << (32 - prefixLength);
    return QHostAddress(mask).toString();
}

static OperationResult readStaticDnsConfig(const QString& protocol,
                                           const QString& guid,
                                           DnsConfig& config)
{
    const QString keyPath = QStringLiteral(
        "SYSTEM\\CurrentControlSet\\Services\\%1\\Parameters\\Interfaces\\%2")
        .arg(protocol, guid);

    HKEY key = nullptr;
    LONG status = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        reinterpret_cast<const wchar_t*>(keyPath.utf16()),
        0,
        KEY_QUERY_VALUE,
        &key);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) {
        config.automatic = true;
        return OperationResult::success();
    }
    if (status != ERROR_SUCCESS) {
        return OperationResult::failure(
            status,
            QObject::tr("Failed to read DNS configuration (error %1).").arg(status));
    }

    DWORD type = 0;
    DWORD byteCount = 0;
    status = RegQueryValueExW(key, L"NameServer", nullptr, &type, nullptr, &byteCount);
    if (status == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(key);
        config.automatic = true;
        return OperationResult::success();
    }
    if (status != ERROR_SUCCESS) {
        RegCloseKey(key);
        return OperationResult::failure(
            status,
            QObject::tr("Failed to read DNS configuration (error %1).").arg(status));
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        RegCloseKey(key);
        return OperationResult::failure(
            ERROR_INVALID_DATATYPE,
            QObject::tr("DNS configuration has an unsupported registry type."));
    }

    QVector<wchar_t> buffer(static_cast<int>(byteCount / sizeof(wchar_t)) + 1, L'\0');
    status = RegQueryValueExW(
        key,
        L"NameServer",
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(buffer.data()),
        &byteCount);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) {
        return OperationResult::failure(
            status,
            QObject::tr("Failed to read DNS configuration (error %1).").arg(status));
    }

    const QString value = QString::fromWCharArray(buffer.constData()).trimmed();
    config.automatic = value.isEmpty();
    if (!config.automatic) {
        config.servers = value.split(
            QRegularExpression(QStringLiteral("[,;\\s]+")),
            Qt::SkipEmptyParts);
    }
    return OperationResult::success();
}

static OperationResult captureNetworkSnapshot(const AdapterInfo& adapter,
                                              NetworkSnapshot& snapshot)
{
    ULONG bufferLength = 16 * 1024;
    QByteArray buffer(static_cast<int>(bufferLength), '\0');
    DWORD status = GetAdaptersAddresses(
        AF_UNSPEC,
        GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS |
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
        nullptr,
        reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
        &bufferLength);
    if (status == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(static_cast<int>(bufferLength));
        status = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS |
                GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
            nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
            &bufferLength);
    }
    if (status != NO_ERROR) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to capture current adapter configuration (error %1).")
                .arg(status));
    }

    IP_ADAPTER_ADDRESSES* selected = nullptr;
    for (auto* current = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
         current;
         current = current->Next) {
        if (current->Luid.Value == adapter.luid.Value) {
            selected = current;
            break;
        }
    }
    if (!selected) {
        return OperationResult::failure(
            ERROR_NOT_FOUND,
            QObject::tr("Adapter disappeared while capturing its configuration."));
    }

    snapshot.ipv4.dhcp = (selected->Flags & IP_ADAPTER_DHCP_ENABLED) != 0;
    for (auto* address = selected->FirstUnicastAddress;
         address;
         address = address->Next) {
        if (!address->Address.lpSockaddr ||
            address->Address.lpSockaddr->sa_family != AF_INET) {
            continue;
        }
        const auto* ipv4 =
            reinterpret_cast<const sockaddr_in*>(address->Address.lpSockaddr);
        snapshot.ipv4.address =
            QHostAddress(ntohl(ipv4->sin_addr.s_addr)).toString();
        snapshot.ipv4.subnetMask = prefixToMask(address->OnLinkPrefixLength);
        break;
    }
    for (auto* gateway = selected->FirstGatewayAddress;
         gateway;
         gateway = gateway->Next) {
        if (!gateway->Address.lpSockaddr ||
            gateway->Address.lpSockaddr->sa_family != AF_INET) {
            continue;
        }
        const auto* ipv4 =
            reinterpret_cast<const sockaddr_in*>(gateway->Address.lpSockaddr);
        snapshot.ipv4.gateway =
            QHostAddress(ntohl(ipv4->sin_addr.s_addr)).toString();
        break;
    }

    for (auto* dns = selected->FirstDnsServerAddress; dns; dns = dns->Next) {
        if (!dns->Address.lpSockaddr)
            continue;
        if (dns->Address.lpSockaddr->sa_family == AF_INET) {
            const auto* ipv4 =
                reinterpret_cast<const sockaddr_in*>(dns->Address.lpSockaddr);
            snapshot.ipv4.dns.servers.append(
                QHostAddress(ntohl(ipv4->sin_addr.s_addr)).toString());
        } else if (dns->Address.lpSockaddr->sa_family == AF_INET6) {
            const auto* ipv6 =
                reinterpret_cast<const sockaddr_in6*>(dns->Address.lpSockaddr);
            Q_IPV6ADDR address = {};
            memcpy(&address, &ipv6->sin6_addr, sizeof(address));
            snapshot.ipv6Dns.servers.append(QHostAddress(address).toString());
        }
    }

    OperationResult result =
        readStaticDnsConfig(QStringLiteral("Tcpip"), adapter.guid, snapshot.ipv4.dns);
    if (!result.ok)
        return result;
    result =
        readStaticDnsConfig(QStringLiteral("Tcpip6"), adapter.guid, snapshot.ipv6Dns);
    if (!result.ok)
        return result;

    PMIB_UNICASTIPADDRESS_TABLE addressTable = nullptr;
    status = GetUnicastIpAddressTable(AF_INET6, &addressTable);
    if (status != NO_ERROR || !addressTable) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to capture IPv6 addresses (error %1).").arg(status));
    }
    for (DWORD index = 0; index < addressTable->NumEntries; ++index) {
        const auto& row = addressTable->Table[index];
        if (row.InterfaceLuid.Value == adapter.luid.Value &&
            (row.PrefixOrigin == IpPrefixOriginManual ||
             row.SuffixOrigin == IpSuffixOriginManual)) {
            snapshot.manualIpv6Addresses.append(row);
        }
    }
    FreeMibTable(addressTable);

    PMIB_IPFORWARD_TABLE2 routeTable = nullptr;
    status = GetIpForwardTable2(AF_INET6, &routeTable);
    if (status != NO_ERROR || !routeTable) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to capture IPv6 routes (error %1).").arg(status));
    }
    for (DWORD index = 0; index < routeTable->NumEntries; ++index) {
        const auto& row = routeTable->Table[index];
        if (row.InterfaceLuid.Value == adapter.luid.Value &&
            row.DestinationPrefix.Prefix.si_family == AF_INET6 &&
            row.DestinationPrefix.PrefixLength == 0 &&
            row.Protocol == MIB_IPPROTO_NETMGMT) {
            snapshot.manualIpv6DefaultRoutes.append(row);
        }
    }
    FreeMibTable(routeTable);

    if (!snapshot.ipv4.dhcp &&
        (snapshot.ipv4.address.isEmpty() || snapshot.ipv4.subnetMask.isEmpty())) {
        return OperationResult::failure(
            ERROR_INVALID_DATA,
            QObject::tr("Current static IPv4 configuration cannot be snapshotted safely."));
    }

    return OperationResult::success();
}

static OperationResult configureDns(const QString& protocol,
                                    const QString& deviceName,
                                    const DnsConfig& config)
{
    QStringList arguments = {
        QStringLiteral("interface"),
        protocol,
        QStringLiteral("set"),
        QStringLiteral("dnsservers"),
        QStringLiteral("name=") + deviceName
    };

    if (config.automatic) {
        arguments << QStringLiteral("source=dhcp");
    } else {
        arguments << QStringLiteral("source=static")
                  << QStringLiteral("address=") +
                         (config.servers.isEmpty()
                              ? QStringLiteral("none")
                              : config.servers.first())
                  << QStringLiteral("validate=no");
    }

    OperationResult result = runNetsh(arguments);
    if (!result.ok) {
        result.message = QObject::tr("Failed to configure %1 DNS: %2")
            .arg(protocol.toUpper(), result.message);
        return result;
    }

    if (config.automatic || config.servers.size() <= 1)
        return result;

    for (int index = 1; index < config.servers.size(); ++index) {
        result = runNetsh({
            QStringLiteral("interface"),
            protocol,
            QStringLiteral("add"),
            QStringLiteral("dnsservers"),
            QStringLiteral("name=") + deviceName,
            QStringLiteral("address=") + config.servers[index],
            QStringLiteral("index=") + QString::number(index + 1),
            QStringLiteral("validate=no")
        });
        if (!result.ok) {
            result.message = QObject::tr("Failed to add %1 DNS server %2: %3")
                .arg(protocol.toUpper(), config.servers[index], result.message);
            return result;
        }
    }

    return OperationResult::success();
}

static OperationResult configureIpv4(const QString& deviceName,
                                     const Ipv4Config& config)
{
    QStringList arguments = {
        QStringLiteral("interface"),
        QStringLiteral("ipv4"),
        QStringLiteral("set"),
        QStringLiteral("address"),
        QStringLiteral("name=") + deviceName
    };
    if (config.dhcp) {
        arguments << QStringLiteral("source=dhcp")
                  << QStringLiteral("store=persistent");
    } else {
        arguments << QStringLiteral("source=static")
                  << QStringLiteral("address=") + config.address
                  << QStringLiteral("mask=") + config.subnetMask
                  << QStringLiteral("gateway=") +
                         (config.gateway.isEmpty()
                              ? QStringLiteral("none")
                              : config.gateway);
        if (!config.gateway.isEmpty())
            arguments << QStringLiteral("gwmetric=1");
        arguments << QStringLiteral("store=persistent");
    }

    OperationResult result = runNetsh(arguments);
    if (!result.ok) {
        result.message = QObject::tr("Failed to configure IPv4 address: %1")
            .arg(result.message);
        return result;
    }

    result = configureDns(QStringLiteral("ipv4"), deviceName, config.dns);
    return result;
}

static bool isManualIpv6DefaultRoute(const MIB_IPFORWARD_ROW2& row,
                                     const AdapterInfo& adapter)
{
    return row.InterfaceLuid.Value == adapter.luid.Value &&
           row.DestinationPrefix.Prefix.si_family == AF_INET6 &&
           row.DestinationPrefix.PrefixLength == 0 &&
           row.Protocol == MIB_IPPROTO_NETMGMT;
}

static OperationResult removeManualIpv6Configuration(const AdapterInfo& adapter)
{
    PMIB_UNICASTIPADDRESS_TABLE addressTable = nullptr;
    NETIO_STATUS status = GetUnicastIpAddressTable(AF_INET6, &addressTable);
    if (status != NO_ERROR || !addressTable) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to enumerate IPv6 addresses (error %1).").arg(status));
    }

    for (DWORD index = 0; index < addressTable->NumEntries; ++index) {
        auto& row = addressTable->Table[index];
        if (row.InterfaceLuid.Value != adapter.luid.Value ||
            (row.PrefixOrigin != IpPrefixOriginManual &&
             row.SuffixOrigin != IpSuffixOriginManual)) {
            continue;
        }
        status = DeleteUnicastIpAddressEntry(&row);
        if (status != NO_ERROR && status != ERROR_NOT_FOUND) {
            FreeMibTable(addressTable);
            return OperationResult::failure(
                static_cast<int>(status),
                QObject::tr("Failed to remove an existing IPv6 address (error %1).")
                    .arg(status));
        }
    }
    FreeMibTable(addressTable);

    PMIB_IPFORWARD_TABLE2 routeTable = nullptr;
    status = GetIpForwardTable2(AF_INET6, &routeTable);
    if (status != NO_ERROR || !routeTable) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to enumerate IPv6 routes (error %1).").arg(status));
    }
    for (DWORD index = 0; index < routeTable->NumEntries; ++index) {
        auto& row = routeTable->Table[index];
        if (!isManualIpv6DefaultRoute(row, adapter))
            continue;
        status = DeleteIpForwardEntry2(&row);
        if (status != NO_ERROR && status != ERROR_NOT_FOUND) {
            FreeMibTable(routeTable);
            return OperationResult::failure(
                static_cast<int>(status),
                QObject::tr("Failed to remove the existing IPv6 gateway (error %1).")
                    .arg(status));
        }
    }
    FreeMibTable(routeTable);
    return OperationResult::success();
}

static OperationResult createIpv6Address(const AdapterInfo& adapter,
                                         const QString& addressText,
                                         int prefixLength)
{
    const QHostAddress address(addressText);
    if (address.isNull()) {
        return OperationResult::failure(
            ERROR_INVALID_PARAMETER,
            QObject::tr("The IPv6 address is invalid."));
    }

    MIB_UNICASTIPADDRESS_ROW row;
    InitializeUnicastIpAddressEntry(&row);
    row.InterfaceLuid = adapter.luid;
    row.InterfaceIndex = adapter.ifIndex;
    row.Address.Ipv6.sin6_family = AF_INET6;
    const Q_IPV6ADDR bytes = address.toIPv6Address();
    memcpy(&row.Address.Ipv6.sin6_addr, &bytes, sizeof(bytes));
    bool scopeOk = false;
    const uint scopeId = address.scopeId().toUInt(&scopeOk);
    if (scopeOk)
        row.Address.Ipv6.sin6_scope_id = scopeId;
    row.OnLinkPrefixLength = static_cast<UINT8>(prefixLength);
    row.DadState = NldsPreferred;
    row.PrefixOrigin = IpPrefixOriginManual;
    row.SuffixOrigin = IpSuffixOriginManual;

    const NETIO_STATUS status = CreateUnicastIpAddressEntry(&row);
    if (status != NO_ERROR) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to create the IPv6 address (error %1).").arg(status));
    }
    return OperationResult::success();
}

static OperationResult createIpv6DefaultRoute(const AdapterInfo& adapter,
                                              const QString& gatewayText)
{
    if (gatewayText.isEmpty())
        return OperationResult::success();

    const QHostAddress gateway(gatewayText);
    if (gateway.isNull()) {
        return OperationResult::failure(
            ERROR_INVALID_PARAMETER,
            QObject::tr("The IPv6 gateway is invalid."));
    }

    MIB_IPFORWARD_ROW2 row;
    InitializeIpForwardEntry(&row);
    row.InterfaceLuid = adapter.luid;
    row.InterfaceIndex = adapter.ifIndex;
    row.DestinationPrefix.Prefix.si_family = AF_INET6;
    row.DestinationPrefix.PrefixLength = 0;
    row.NextHop.si_family = AF_INET6;
    const Q_IPV6ADDR bytes = gateway.toIPv6Address();
    memcpy(&row.NextHop.Ipv6.sin6_addr, &bytes, sizeof(bytes));
    bool scopeOk = false;
    const uint scopeId = gateway.scopeId().toUInt(&scopeOk);
    row.NextHop.Ipv6.sin6_scope_id = scopeOk ? scopeId : adapter.ifIndex;
    row.Metric = 1;
    row.Protocol = static_cast<NL_ROUTE_PROTOCOL>(MIB_IPPROTO_NETMGMT);
    row.Origin = NlroManual;

    const NETIO_STATUS status = CreateIpForwardEntry2(&row);
    if (status != NO_ERROR) {
        return OperationResult::failure(
            static_cast<int>(status),
            QObject::tr("Failed to create the IPv6 gateway (error %1).").arg(status));
    }
    return OperationResult::success();
}

static OperationResult configureIpv6(const AdapterInfo& adapter,
                                     const QString& deviceName,
                                     const Profile& profile)
{
    OperationResult result = removeManualIpv6Configuration(adapter);
    if (!result.ok)
        return result;

    if (!profile.ipv6Dhcp) {
        result = createIpv6Address(adapter, profile.ipv6Address, profile.ipv6Prefix);
        if (!result.ok)
            return result;
        result = createIpv6DefaultRoute(adapter, profile.ipv6Gateway);
        if (!result.ok)
            return result;
    }

    DnsConfig dns;
    dns.automatic = profile.ipv6Dhcp || profile.ipv6DefaultDns;
    if (!dns.automatic) {
        if (!profile.ipv6PreferredDns.isEmpty())
            dns.servers.append(profile.ipv6PreferredDns);
        if (!profile.ipv6AlternateDns.isEmpty())
            dns.servers.append(profile.ipv6AlternateDns);
    }
    return configureDns(QStringLiteral("ipv6"), deviceName, dns);
}

static OperationResult restoreIpv6Configuration(const AdapterInfo& adapter,
                                                const QString& deviceName,
                                                const NetworkSnapshot& snapshot)
{
    OperationResult result = removeManualIpv6Configuration(adapter);
    if (!result.ok)
        return result;

    for (const auto& saved : snapshot.manualIpv6Addresses) {
        MIB_UNICASTIPADDRESS_ROW row;
        InitializeUnicastIpAddressEntry(&row);
        row.InterfaceLuid = adapter.luid;
        row.InterfaceIndex = adapter.ifIndex;
        row.Address = saved.Address;
        row.OnLinkPrefixLength = saved.OnLinkPrefixLength;
        row.DadState = NldsPreferred;
        row.PrefixOrigin = IpPrefixOriginManual;
        row.SuffixOrigin = IpSuffixOriginManual;
        const NETIO_STATUS status = CreateUnicastIpAddressEntry(&row);
        if (status != NO_ERROR) {
            return OperationResult::failure(
                static_cast<int>(status),
                QObject::tr("Failed to restore an IPv6 address (error %1).").arg(status));
        }
    }

    for (const auto& saved : snapshot.manualIpv6DefaultRoutes) {
        MIB_IPFORWARD_ROW2 row;
        InitializeIpForwardEntry(&row);
        row.InterfaceLuid = adapter.luid;
        row.InterfaceIndex = adapter.ifIndex;
        row.DestinationPrefix = saved.DestinationPrefix;
        row.NextHop = saved.NextHop;
        row.Metric = saved.Metric;
        row.Protocol = static_cast<NL_ROUTE_PROTOCOL>(MIB_IPPROTO_NETMGMT);
        row.Origin = NlroManual;
        const NETIO_STATUS status = CreateIpForwardEntry2(&row);
        if (status != NO_ERROR) {
            return OperationResult::failure(
                static_cast<int>(status),
                QObject::tr("Failed to restore the IPv6 gateway (error %1).").arg(status));
        }
    }

    return configureDns(QStringLiteral("ipv6"), deviceName, snapshot.ipv6Dns);
}

// DnsFlushResolverCache is not declared in MinGW headers
typedef BOOL (WINAPI* DnsFlushFunc)();
static void flushDnsCache()
{
    HMODULE h = LoadLibraryW(L"dnsapi.dll");
    if (!h) return;
    const FARPROC rawFunction = GetProcAddress(h, "DnsFlushResolverCache");
    DnsFlushFunc pfn = nullptr;
    static_assert(sizeof(pfn) == sizeof(rawFunction));
    memcpy(&pfn, &rawFunction, sizeof(pfn));
    if (pfn) pfn();
    FreeLibrary(h);
}

// ======================================================================
//  Apply profile via IP Helper API
// ======================================================================
static QJsonObject applyProfile(const QJsonObject& profileJson)
{
    QJsonObject resp;
    resp["ok"] = false;

    const Profile profile = Profile::fromJson(profileJson);
    QString validationError;
    if (!profile.isValid(&validationError)) {
        resp["summary"] = QObject::tr("Invalid profile: %1").arg(validationError);
        resp["exitCode"] = static_cast<int>(ERROR_INVALID_DATA);
        return resp;
    }

    const AdapterInfo adapter =
        resolveAdapter(profile.deviceGuid, profile.deviceName);
    if (!adapter.valid) {
        resp["summary"] = QObject::tr(
            "Network adapter '%1' is unavailable. "
            "It may have been removed, replaced, or disabled.")
            .arg(profile.deviceName);
        resp["exitCode"] = static_cast<int>(ERROR_NOT_FOUND);
        return resp;
    }

    NetworkSnapshot snapshot;
    OperationResult result = captureNetworkSnapshot(adapter, snapshot);
    if (!result.ok) {
        resp["summary"] = QObject::tr(
            "The current network configuration could not be backed up. "
            "No changes were made.\n\n%1").arg(result.message);
        resp["exitCode"] = result.code;
        return resp;
    }

    Ipv4Config targetIpv4;
    targetIpv4.dhcp = profile.dhcp;
    targetIpv4.address = profile.ipAddress;
    targetIpv4.subnetMask = profile.subnetMask;
    targetIpv4.gateway = profile.gateway;
    targetIpv4.dns.automatic = profile.dhcp || profile.defaultDns;
    if (!targetIpv4.dns.automatic) {
        if (!profile.preferredDns.isEmpty())
            targetIpv4.dns.servers.append(profile.preferredDns);
        if (!profile.alternateDns.isEmpty())
            targetIpv4.dns.servers.append(profile.alternateDns);
    }

    result = configureIpv4(adapter.friendlyName, targetIpv4);
    if (!result.ok) {
        const OperationResult rollback =
            configureIpv4(adapter.friendlyName, snapshot.ipv4);
        QString summary = QObject::tr("Failed to apply IPv4 configuration.\n\n%1")
            .arg(result.message);
        summary += rollback.ok
            ? QObject::tr("\n\nThe previous IPv4 configuration was restored.")
            : QObject::tr("\n\nRollback also failed: %1").arg(rollback.message);
        resp["summary"] = summary;
        resp["exitCode"] = result.code;
        return resp;
    }

    result = configureIpv6(adapter, adapter.friendlyName, profile);
    if (!result.ok) {
        const OperationResult ipv6Rollback =
            restoreIpv6Configuration(adapter, adapter.friendlyName, snapshot);
        const OperationResult ipv4Rollback =
            configureIpv4(adapter.friendlyName, snapshot.ipv4);

        QStringList rollbackFailures;
        if (!ipv6Rollback.ok)
            rollbackFailures.append(ipv6Rollback.message);
        if (!ipv4Rollback.ok)
            rollbackFailures.append(ipv4Rollback.message);

        QString summary = QObject::tr("Failed to apply IPv6 configuration.\n\n%1")
            .arg(result.message);
        summary += rollbackFailures.isEmpty()
            ? QObject::tr("\n\nThe previous network configuration was restored.")
            : QObject::tr("\n\nRollback was incomplete: %1")
                  .arg(rollbackFailures.join(QStringLiteral("; ")));
        resp["summary"] = summary;
        resp["exitCode"] = result.code;
        return resp;
    }

    flushDnsCache();
    resp["ok"] = true;
    resp["summary"] = QObject::tr("Applied profile '%1' on %2.")
        .arg(profile.name, adapter.friendlyName);
    resp["deviceName"] = adapter.friendlyName;
    resp["deviceGuid"] = adapter.guid;
    resp["exitCode"] = 0;
    return resp;
}

// ------------------------------------------------------------------
//  Named pipe helpers
// ------------------------------------------------------------------
static bool readAll(HANDLE h, void* buf, DWORD len)
{
    DWORD total = 0;
    while (total < len) {
        DWORD read;
        if (!ReadFile(h, static_cast<char*>(buf) + total, len - total, &read, nullptr))
            return false;
        total += read;
    }
    return true;
}

// ======================================================================
//  Service implementation
// ======================================================================

IPAssistantService& IPAssistantService::instance()
{
    static IPAssistantService s;
    return s;
}

IPAssistantService::~IPAssistantService()
{
    if (m_thread) CloseHandle(m_thread);
    if (m_stopEvent) CloseHandle(m_stopEvent);
}

bool IPAssistantService::start(LPCWSTR serviceName)
{
    m_serviceName = QString::fromWCharArray(serviceName);
    g_serviceSelf = this;

    SERVICE_TABLE_ENTRYW dispatchTable[] = {
        { const_cast<LPWSTR>(serviceName), serviceMainImpl },
        { nullptr, nullptr }
    };
    return StartServiceCtrlDispatcherW(dispatchTable) != FALSE;
}

void IPAssistantService::stop()
{
    onStop();
}

void IPAssistantService::onStart(DWORD, LPWSTR*)
{
    m_statusHandle = RegisterServiceCtrlHandlerExW(
        reinterpret_cast<const wchar_t*>(m_serviceName.utf16()),
        ctrlHandlerImpl,
        nullptr);

    if (!m_statusHandle) return;

    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 1;
    m_status.dwWaitHint = 5000;
    SetServiceStatus(m_statusHandle, &m_status);

    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent) {
        m_status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(m_statusHandle, &m_status);
        return;
    }

    m_thread = CreateThread(nullptr, 0, pipeThreadProc, this, 0, nullptr);
    if (!m_thread) {
        CloseHandle(m_stopEvent); m_stopEvent = nullptr;
        m_status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(m_statusHandle, &m_status);
        return;
    }

    m_running = true;
    m_status.dwCurrentState = SERVICE_RUNNING;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
    SetServiceStatus(m_statusHandle, &m_status);
}

void IPAssistantService::onStop()
{
    if (!m_running) return;
    m_running = false;

    m_status.dwCurrentState = SERVICE_STOP_PENDING;
    m_status.dwCheckPoint = 2;
    m_status.dwWaitHint = 5000;
    SetServiceStatus(m_statusHandle, &m_status);

    if (m_stopEvent) SetEvent(m_stopEvent);
    if (m_thread) {
        WaitForSingleObject(m_thread, 3000);
        CloseHandle(m_thread); m_thread = nullptr;
    }
    if (m_stopEvent) {
        CloseHandle(m_stopEvent); m_stopEvent = nullptr;
    }

    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwCheckPoint = 0;
    SetServiceStatus(m_statusHandle, &m_status);
}

DWORD WINAPI IPAssistantService::pipeThreadProc(LPVOID param)
{
    auto* self = static_cast<IPAssistantService*>(param);
    self->pipeServerLoop();
    return 0;
}

void IPAssistantService::pipeServerLoop()
{
    // Security descriptor: allow Authenticated Users to connect
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = nullptr;

    ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;AU)", SDDL_REVISION_1,
        &sa.lpSecurityDescriptor, nullptr);

    while (m_running) {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 65536, 65536, 0,
            sa.lpSecurityDescriptor ? &sa : nullptr);

        if (hPipe == INVALID_HANDLE_VALUE) {
            qWarning() << "CreateNamedPipe failed:" << GetLastError();
            Sleep(1000);
            continue;
        }

        OVERLAPPED ov = {};
        ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        BOOL connected = ConnectNamedPipe(hPipe, &ov);
        if (connected == 0 && GetLastError() == ERROR_IO_PENDING) {
            HANDLE events[2] = { ov.hEvent, m_stopEvent };
            DWORD wait = WaitForMultipleObjects(2, events, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1) {
                CancelIo(hPipe);
                CloseHandle(ov.hEvent);
                CloseHandle(hPipe);
                break;
            }
            DWORD dummy;
            GetOverlappedResult(hPipe, &ov, &dummy, TRUE);
        }
        CloseHandle(ov.hEvent);

        if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
            CloseHandle(hPipe);
            continue;
        }

        DWORD payloadLen = 0;
        if (!readAll(hPipe, &payloadLen, 4)) {
            CloseHandle(hPipe);
            continue;
        }

        QByteArray buf(static_cast<int>(payloadLen), Qt::Uninitialized);
        if (!readAll(hPipe, buf.data(), payloadLen)) {
            CloseHandle(hPipe);
            continue;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(buf, &err);
        QJsonObject response;
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            response["ok"] = false;
            response["summary"] = "Invalid JSON request";
        } else {
            QJsonObject req = doc.object();
            QString cmd = req.value("cmd").toString();
            if (cmd == "apply") {
                QJsonObject profile = req.value("profile").toObject();
                response = applyProfile(profile);
            } else {
                response["ok"] = false;
                response["summary"] = "Unknown command: " + cmd;
            }
        }

        QByteArray respData = QJsonDocument(response).toJson(QJsonDocument::Compact);
        DWORD respLen = static_cast<DWORD>(respData.size());
        DWORD written;
        WriteFile(hPipe, &respLen, 4, &written, nullptr);
        WriteFile(hPipe, respData.constData(), respLen, &written, nullptr);
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    if (sa.lpSecurityDescriptor)
        LocalFree(sa.lpSecurityDescriptor);
}
