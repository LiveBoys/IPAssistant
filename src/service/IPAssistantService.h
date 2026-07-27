#ifndef IPASSISTANT_SERVICE_H
#define IPASSISTANT_SERVICE_H

#include <QString>
#include <windows.h>

class IPAssistantService
{
public:
    static IPAssistantService& instance();

    bool start(LPCWSTR serviceName);
    void stop();
    bool isRunning() const { return m_running; }

    void onStart(DWORD argc, LPWSTR* argv);
    void onStop();

private:
    IPAssistantService() = default;
    ~IPAssistantService();
    IPAssistantService(const IPAssistantService&) = delete;
    IPAssistantService& operator=(const IPAssistantService&) = delete;

    static DWORD WINAPI pipeThreadProc(LPVOID param);
    void pipeServerLoop();

    HANDLE m_stopEvent = nullptr;
    HANDLE m_thread = nullptr;
    SERVICE_STATUS_HANDLE m_statusHandle = nullptr;
    SERVICE_STATUS m_status = {};
    bool m_running = false;
    QString m_serviceName;
};

#endif
