#ifndef IPASSISTANT_CORE_H
#define IPASSISTANT_CORE_H

#include "Profile.h"

#include <QString>

/**
 * @brief Applies a Profile to a real network adapter.
 *
 * We shell out to `netsh` on Windows because it is the only API that
 * consistently works without elevated UAC from a normal user context for
 * the common IPv4 / DNS settings used here. The netsh process is run with
 * CREATE_NO_WINDOW so no console flashes during switching.
 */
class IPAssistantCore
{
public:
    struct Result
    {
        bool        ok = false;
        int         exitCode = -1;
        QString     stdoutText;
        QString     stderrText;
        QString     summary;   ///< Short human-readable description.
    };

    /// Apply the given profile to its bound device.
    static Result apply(const Profile& profile);

private:
    static Result runNetsh(const QStringList& args);
};

#endif // IPASSISTANT_CORE_H