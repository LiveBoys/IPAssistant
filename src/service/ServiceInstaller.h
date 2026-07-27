#ifndef IPASSISTANT_SERVICE_INSTALLER_H
#define IPASSISTANT_SERVICE_INSTALLER_H

#include <QString>

bool installService();
bool uninstallService();
bool startService();
bool stopService();
bool isServiceInstalled();
bool isServiceRunning();
bool installAndStartService();

#endif
