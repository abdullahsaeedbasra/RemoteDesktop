#include "Service.h"
#include "Logger.h"
#include "Listener.h"
#include "ProcessManager.h"

#include <tchar.h>
#include <string>

RemoteDesktopService* RemoteDesktopService::s_Instance = nullptr;

RemoteDesktopService& RemoteDesktopService::Instance() {
    static RemoteDesktopService instance;
    s_Instance = &instance;
    return instance;
}

void RemoteDesktopService::Run()
{
    SERVICE_TABLE_ENTRY table[] =
    {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { nullptr, nullptr }
    };

    StartServiceCtrlDispatcher(table);
}

VOID WINAPI RemoteDesktopService::ServiceCtrlHandler(DWORD ctrlCode)
{
    auto& svc = Instance();

    if (ctrlCode == SERVICE_CONTROL_STOP)
    {
        svc.m_Status.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);

        SetEvent(svc.m_StopEvent);
    }
}

VOID WINAPI RemoteDesktopService::ServiceMain(DWORD, LPWSTR*)
{
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        Log("WSAStartup failed");
        return;
    }

    auto& svc = Instance();

    svc.m_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (!svc.m_StatusHandle)
    {
        WSACleanup();
        return;
    }

    svc.m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    svc.m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    svc.m_Status.dwCurrentState = SERVICE_START_PENDING;

    SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);

    svc.m_StopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (svc.m_StopEvent == nullptr)
    {
        svc.m_Status.dwCurrentState = SERVICE_STOPPED;
        svc.m_Status.dwWin32ExitCode = GetLastError();
        SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
        WSACleanup();
        return;
    }

    svc.m_Status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);

    HANDLE hClientConnected = CreateEvent(nullptr, FALSE, FALSE, L"ClientConnected");
    HANDLE hClientDisconnected = CreateEvent(nullptr, FALSE, FALSE, L"ClientDisconnected");
    HANDLE hClientDisconnectedListener = CreateEvent(nullptr, FALSE, FALSE, L"ClientDisconnectedListener");
    Sleep(50);

    Listener* pListener = nullptr;
    ProcessManager* pProcessManager = nullptr;

    pListener = (Listener*)AfxBeginThread(RUNTIME_CLASS(Listener), THREAD_PRIORITY_NORMAL);

    HANDLE hEvents[3];
    hEvents[0] = svc.m_StopEvent;
    hEvents[1] = hClientConnected;
    hEvents[2] = hClientDisconnected;

    while (true)
    {
        int index = ::WaitForMultipleObjects(3, hEvents, FALSE, INFINITE) - WAIT_OBJECT_0;

        if (index == 0)
        {
            Log("Service Main: Stop Event Signaled");

            if (pProcessManager)
            {
                pProcessManager->SignalStopWhenClientIsConnected();
                ::WaitForSingleObject(pProcessManager->m_hStopped, INFINITE);
                pProcessManager = nullptr;
            }

            if (pListener)
            {
                pListener->SignalStop();
                ::WaitForSingleObject(pListener->m_hStopped, INFINITE);
                pListener = nullptr;
            }

            if (hClientConnected != nullptr)
                CloseHandle(hClientConnected);
            if (hClientDisconnected != nullptr)
                CloseHandle(hClientDisconnected);

            svc.m_Status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
            WSACleanup();
            Sleep(100);
            return;
        }
        else if (index == 1)
        {
            Log("Service Main: Client Connected");
            pProcessManager = nullptr;
            pProcessManager = (ProcessManager*)AfxBeginThread(RUNTIME_CLASS(ProcessManager), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
            pProcessManager->SetSocket(pListener->GetSocket());
            pProcessManager->ResumeThread();
        }
        else
        {
            Log("Service Main: Client Disconnected");
            if (pProcessManager)
            {
                pProcessManager->SignalStop();
                ::WaitForSingleObject(pProcessManager->m_hStopped, INFINITE);
                pProcessManager = nullptr;
            }
            SetEvent(hClientDisconnectedListener);
        }
    }
}