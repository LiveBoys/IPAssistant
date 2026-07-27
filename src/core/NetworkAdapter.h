#ifndef IPSWITCH_NETWORK_ADAPTER_H
#define IPSWITCH_NETWORK_ADAPTER_H

#include "Profile.h"

#include <QString>
#include <QStringList>

struct NetworkAdapterBinding
{
    QString name;
    QString guid;

    bool isValid() const { return !name.isEmpty(); }
};

/**
 * @brief Enumerates physical / virtual network adapters available on the host.
 *
 * On Windows we use IP Helper (GetAdaptersAddresses). On non-Windows
 * platforms we fall back to QNetworkInterface, which is enough to keep the
 * UI populated for development outside Windows.
 */
class NetworkAdapter
{
public:
    /// Returns a list of friendly adapter names suitable for display.
    static QStringList enumerate();

    /// Resolve a friendly name to its underlying interface GUID/name used by netsh.
    /// Returns an empty string if not found.
    static QString interfaceNameFor(const QString& friendlyName);

    /// Return the stable adapter GUID (AdapterName) for a given friendly name.
    /// This matches WMI Win32_NetworkAdapterConfiguration.SettingID.
    static QString interfaceGuidFor(const QString& friendlyName);

    /// Resolve an adapter by stable GUID first, then by friendly-name fallback.
    static NetworkAdapterBinding resolve(const QString& guid,
                                         const QString& fallbackName = {});

    /// Return the current friendly name for a stable adapter GUID.
    static QString friendlyNameForGuid(const QString& guid);

    /// Read the current IPv4 configuration of the given adapter into a Profile.
    /// The returned profile's name/deviceName are filled; callers should only
    /// use the IP/DNS fields.
    static Profile queryCurrentConfig(const QString& friendlyName);
};

#endif // IPSWITCH_NETWORK_ADAPTER_H
