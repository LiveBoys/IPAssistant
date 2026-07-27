#include "ServiceInstaller.h"
#include <windows.h>
#include <winsvc.h>
#include <QDebug>

static const wchar_t* SERVICE_NAME = L"IPAssistantSvc";
static const wchar_t* SERVICE_DISPLAY = L"IP Assistant Service";
static const wchar_t* SERVICE_DESC = L"The IP Assistant service enables IP address switching in the background.";

static wchar_t selfPath[MAX_PATH + 16] = {};

static const wchar_t* getSelfPath()
{
    if (!selfPath[0]) {
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);
    }
    return selfPath;
}

bool installService()
{
    wchar_t path[MAX_PATH + 32] = {};
    wcsncpy_s(path, getSelfPath(), _TRUNCATE);

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        qWarning() << "OpenSCManager failed:" << GetLastError();
        return false;
    }

    SC_HANDLE svc = CreateServiceW(
        scm, SERVICE_NAME, SERVICE_DISPLAY,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        path, nullptr, nullptr, nullptr, nullptr, nullptr);

    DWORD err = GetLastError();

    if (svc) {
        SERVICE_DESCRIPTIONW desc = { const_cast<LPWSTR>(SERVICE_DESC) };
        ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (!svc && err != ERROR_SERVICE_EXISTS) {
        qWarning() << "CreateService failed:" << err;
        return false;
    }
    return true;
}

bool uninstallService()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (!svc) { CloseServiceHandle(scm); return false; }

    // Stop first
    SERVICE_STATUS ss;
    ControlService(svc, SERVICE_CONTROL_STOP, &ss);

    bool ok = DeleteService(svc);
    if (!ok) qWarning() << "DeleteService failed:" << GetLastError();
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

bool startService()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_START | SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(scm); return false; }

    SERVICE_STATUS ss;
    if (QueryServiceStatus(svc, &ss) && ss.dwCurrentState == SERVICE_RUNNING) {
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return true;
    }

    bool ok = StartServiceW(svc, 0, nullptr);
    if (!ok) qWarning() << "StartService failed:" << GetLastError();
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

bool stopService()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(scm); return false; }

    SERVICE_STATUS ss;
    ControlService(svc, SERVICE_CONTROL_STOP, &ss);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

bool isServiceInstalled()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_QUERY_STATUS);
    CloseServiceHandle(scm);
    if (!svc) return false;
    CloseServiceHandle(svc);
    return true;
}

bool isServiceRunning()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(scm); return false; }

    SERVICE_STATUS ss;
    bool running = QueryServiceStatus(svc, &ss) && ss.dwCurrentState == SERVICE_RUNNING;
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return running;
}

bool installAndStartService()
{
    if (!installService()) return false;
    // Small delay to let SCM register
    Sleep(500);
    return startService();
}
