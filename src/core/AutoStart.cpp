#include "AutoStart.h"

#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace {
constexpr const char* kRunKeyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
}

QString AutoStart::registryValueName()
{
    return QStringLiteral("IPAssistant");
}

bool AutoStart::isEnabled()
{
#ifdef Q_OS_WIN
    HKEY hKey = nullptr;
    QString keyPath = QString::fromUtf8(kRunKeyPath);
    LONG rc = RegOpenKeyExW(HKEY_CURRENT_USER,
                            reinterpret_cast<LPCWSTR>(keyPath.utf16()),
                            0,
                            KEY_READ,
                            &hKey);
    if (rc != ERROR_SUCCESS) return false;

    QString valueName = registryValueName();
    // Existence check: ERROR_SUCCESS means the value is present.
    rc = RegQueryValueExW(hKey,
                          reinterpret_cast<LPCWSTR>(valueName.utf16()),
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr);
    RegCloseKey(hKey);
    return rc == ERROR_SUCCESS;
#else
    return false;
#endif
}

bool AutoStart::setEnabled(bool enable)
{
#ifdef Q_OS_WIN
    HKEY hKey = nullptr;
    QString keyPath = QString::fromUtf8(kRunKeyPath);
    LONG rc = RegOpenKeyExW(HKEY_CURRENT_USER,
                            reinterpret_cast<LPCWSTR>(keyPath.utf16()),
                            0,
                            KEY_SET_VALUE,
                            &hKey);
    if (rc != ERROR_SUCCESS) return false;

    QString valueName = registryValueName();
    bool ok = true;
    if (enable) {
        // Quote the path to be safe with spaces.
        QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        QString cmd = QStringLiteral("\"%1\"").arg(exe);
        // Windows: sizeof(wchar_t) == 2 on all supported platforms, so the
        // raw buffer of QString::utf16() is compatible with the Win32 W APIs.
        const BYTE* data = reinterpret_cast<const BYTE*>(cmd.utf16());
        DWORD bytes = static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t));
        rc = RegSetValueExW(hKey,
                            reinterpret_cast<LPCWSTR>(valueName.utf16()),
                            0,
                            REG_SZ,
                            data,
                            bytes);
        ok = (rc == ERROR_SUCCESS);
    } else {
        rc = RegDeleteValueW(hKey,
                             reinterpret_cast<LPCWSTR>(valueName.utf16()));
        ok = (rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND);
    }
    RegCloseKey(hKey);
    return ok;
#else
    (void)enable;
    return false;
#endif
}