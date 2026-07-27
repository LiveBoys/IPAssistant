#include "IPAssistantService.h"
#include "ServiceInstaller.h"

#include <cstdio>

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse args before any Qt init
    bool hasInstall = false, hasUninstall = false;
    bool hasStart = false, hasStop = false, hasStatus = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--install")   hasInstall = true;
        if (arg == "--uninstall") hasUninstall = true;
        if (arg == "--start")     hasStart = true;
        if (arg == "--stop")      hasStop = true;
        if (arg == "--status")    hasStatus = true;
    }

    if (hasStatus) {
        const char* s = "NOT_INSTALLED";
        if (isServiceRunning())       s = "RUNNING";
        else if (isServiceInstalled()) s = "STOPPED";
        std::printf("%s\n", s);
        return 0;
    }
    if (hasInstall) {
        bool ok = installAndStartService();
        std::printf("%s\n", ok ? "OK" : "FAIL");
        return ok ? 0 : 1;
    }
    if (hasUninstall) {
        bool ok = uninstallService();
        std::printf("%s\n", ok ? "OK" : "FAIL");
        return ok ? 0 : 1;
    }
    if (hasStart) {
        bool ok = startService();
        std::printf("%s\n", ok ? "OK" : "FAIL");
        return ok ? 0 : 1;
    }
    if (hasStop) {
        stopService();
        std::printf("OK\n");
        return 0;
    }

    // Run as Windows Service
    if (!IPAssistantService::instance().start(L"IPAssistantSvc")) {
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            std::fprintf(stderr, "Not running as a service. Use --install, --start, etc.\n");
        } else {
            std::fprintf(stderr, "StartServiceCtrlDispatcher failed: %lu\n", err);
        }
        return 1;
    }
    return 0;
}
