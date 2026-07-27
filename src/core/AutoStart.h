#ifndef IPASSISTANT_AUTOSTART_H
#define IPASSISTANT_AUTOSTART_H

#include <QString>

/**
 * @brief Toggle "Start with Windows" via the per-user Run registry key.
 *
 * Uses HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run.
 * This avoids UAC prompts because we only write to the user hive.
 */
class AutoStart
{
public:
    /// True if the per-user Run key already points at this executable.
    static bool isEnabled();

    /// Enable or disable startup entry. Returns true on success.
    static bool setEnabled(bool enable);

private:
    static QString registryValueName();
};

#endif // IPASSISTANT_AUTOSTART_H