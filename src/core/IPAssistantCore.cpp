#include "IPAssistantCore.h"
#include "service/ServiceInstaller.h"
#include "core/NetworkAdapter.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\IPAssistantService";

// ------------------------------------------------------------------
//  Named pipe client helpers
// ------------------------------------------------------------------
#ifdef Q_OS_WIN

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

/// Connect to the named pipe and send/receive JSON. Returns true on success.
static bool pipeExchange(const QJsonObject& request, QJsonObject& response, DWORD timeoutMs)
{
    // Wait for pipe to become available
    if (!WaitNamedPipeW(PIPE_NAME, timeoutMs))
        return false;

    HANDLE hPipe = CreateFileW(PIPE_NAME, GENERIC_READ | GENERIC_WRITE,
                               0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPipe == INVALID_HANDLE_VALUE)
        return false;

    // Send request
    QByteArray reqData = QJsonDocument(request).toJson(QJsonDocument::Compact);
    DWORD len = static_cast<DWORD>(reqData.size());
    DWORD written;
    WriteFile(hPipe, &len, 4, &written, nullptr);
    WriteFile(hPipe, reqData.constData(), len, &written, nullptr);

    // Read response
    DWORD respLen = 0;
    if (!readAll(hPipe, &respLen, 4)) {
        CloseHandle(hPipe);
        return false;
    }

    QByteArray respData(static_cast<int>(respLen), Qt::Uninitialized);
    if (!readAll(hPipe, respData.data(), respLen)) {
        CloseHandle(hPipe);
        return false;
    }

    CloseHandle(hPipe);

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(respData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    response = doc.object();
    return true;
}

/// Run IPAssistantService.exe with given args (elevated via shell runas).
static bool elevatedRun(const wchar_t* args)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    // Replace IPAssistant.exe with IPAssistantService.exe in the path
    wchar_t* p = wcsrchr(path, L'\\');
    if (!p) return false;
    wcsncpy_s(p + 1, MAX_PATH - (p - path) - 1, L"IPAssistantService.exe", _TRUNCATE);

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = path;
    sei.lpParameters = args;
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (!ShellExecuteExW(&sei))
        return false;

    WaitForSingleObject(sei.hProcess, 60000);
    DWORD exitCode;
    GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);
    return exitCode == 0;
}

#endif // Q_OS_WIN

// ======================================================================
//  Public API
// ======================================================================

IPAssistantCore::Result IPAssistantCore::apply(const Profile& p)
{
    // Validate
    QString reason;
    if (!p.isValid(&reason)) {
        Result r;
        r.summary = QObject::tr("Invalid profile: ") + reason;
        return r;
    }

#ifdef Q_OS_WIN
    // Build JSON request
    QJsonObject req;
    req["cmd"] = "apply";
    req["profile"] = p.toJson();

    // Try pipe (quick attempt)
    QJsonObject resp;
    if (pipeExchange(req, resp, 1000)) {
        Result r;
        r.ok = resp.value("ok").toBool();
        r.summary = resp.value("summary").toString();
        r.exitCode = resp.value("exitCode").toInt(-1);
        return r;
    }

    // Pipe not available — try to start the service
    if (!isServiceInstalled()) {
        // Install + start (one UAC pop)
        if (!elevatedRun(L"--install")) {
            Result r;
            r.summary = QObject::tr("Failed to install service. Please run as Administrator once.");
            return r;
        }
        // Wait for service to start
        Sleep(2000);
    } else if (!isServiceRunning()) {
        if (!elevatedRun(L"--start")) {
            Result r;
            r.summary = QObject::tr("Failed to start service. Please run as Administrator once.");
            return r;
        }
        Sleep(1000);
    }

    // Retry pipe with longer timeout
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (pipeExchange(req, resp, 3000)) {
            Result r;
            r.ok = resp.value("ok").toBool();
            r.summary = resp.value("summary").toString();
            r.exitCode = resp.value("exitCode").toInt(-1);
            return r;
        }
        Sleep(500);
    }

    Result r;
    r.summary = QObject::tr("Cannot connect to IPAssistantService. Please run as Administrator once.");
    return r;

#else
    // Non-Windows fallback — QProcess + netsh
    auto runNetsh = [](const QStringList& args) -> Result {
        Result r;
        QProcess proc;
        proc.setProcessChannelMode(QProcess::SeparateChannels);
        proc.start("netsh", args);
        if (!proc.waitForStarted(5000)) {
            r.stderrText = "Failed to start netsh: " + proc.errorString();
            r.summary = r.stderrText;
            return r;
        }
        if (!proc.waitForFinished(15000)) {
            proc.kill();
            r.stderrText = "netsh timed out.";
            r.summary = r.stderrText;
            return r;
        }
        r.exitCode = proc.exitCode();
        r.stdoutText = QString::fromLocal8Bit(proc.readAllStandardOutput());
        r.stderrText = QString::fromLocal8Bit(proc.readAllStandardError());
        r.ok = (r.exitCode == 0);
        if (!r.ok) {
            r.summary = r.stderrText.isEmpty() ? r.stdoutText : r.stderrText;
            if (r.summary.isEmpty())
                r.summary = "netsh failed with code " + QString::number(r.exitCode);
        }
        return r;
    };

    QString ifaceName = NetworkAdapter::interfaceNameFor(p.deviceName);
    if (ifaceName.isEmpty())
        ifaceName = p.deviceName;

    QStringList args;
    args << "interface" << "ipv4" << "set" << "address"
         << "name=" + ifaceName;
    if (p.dhcp) {
        args << "dhcp";
    } else {
        args << "static" << p.ipAddress << p.subnetMask
             << (p.gateway.isEmpty() ? QStringLiteral("none") : p.gateway)
             << "1";
    }
    Result r = runNetsh(args);
    if (!r.ok) return r;

    QStringList dnsResetArgs = {
        "interface", "ipv4", "set", "dnsserver",
        "name=" + ifaceName, "dhcp"
    };
    Result dnsResetResult = runNetsh(dnsResetArgs);

    if (p.dhcp) {
        if (!dnsResetResult.ok)
            qWarning() << "DNS reset to DHCP failed:" << dnsResetResult.summary;
    } else if (!p.defaultDns) {
        if (!p.preferredDns.isEmpty()) {
            QStringList a = {
                "interface", "ipv4", "set", "dnsserver",
                "name=" + ifaceName, "static", p.preferredDns, "primary"
            };
            Result dr = runNetsh(a);
            if (!dr.ok) return dr;
        }
        if (!p.alternateDns.isEmpty()) {
            QStringList a = {
                "interface", "ipv4", "add", "dnsserver",
                "name=" + ifaceName, p.alternateDns, "index=2"
            };
            runNetsh(a);
        }
    }

    r.summary = QObject::tr("Applied profile '%1' on %2.").arg(p.name, p.deviceName);
    r.ok = true;
    return r;
#endif
}
