#include "Service.h"
#include "Logger.h"
#include "Sender.h"
#include "Receiver.h"
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

    /*--------------------------------------------------*/

    HANDLE hPipe = svc.CreatePipe();
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(svc.m_StopEvent, INFINITE);

        svc.m_Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
        WSACleanup();

        return;
    }

    Sender* pSender = (Sender*)AfxBeginThread(RUNTIME_CLASS(Sender), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    ProcessManager* pProcess = (ProcessManager*)AfxBeginThread(RUNTIME_CLASS(ProcessManager), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    Receiver* pReceiver = (Receiver*)AfxBeginThread(RUNTIME_CLASS(Receiver), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

    pSender->SetPipe(hPipe);
    pSender->CreateClientEvent();
    pSender->CreateStopEvent();
    pSender->CreateStoppedEvent();
    pSender->ResumeThread();

    pReceiver->SetClientSocket(pSender->GetCLientSocket());
    pReceiver->CreateStopEvent();
    pReceiver->CreateStoppedEvent();
    pReceiver->SetPipe(hPipe);

    pProcess->CreateStartEvent();
    pProcess->CreateStartFailedEvent();
    pProcess->CreateStopEvent();
    pProcess->CreateStoppedEvent();

    HANDLE hEvents1[2];
    hEvents1[0] = svc.m_StopEvent;
    hEvents1[1] = pSender->GetClientEvent();

    int index = WaitForMultipleObjects(2, hEvents1, FALSE, INFINITE) - WAIT_OBJECT_0;
    Log("Client Event Index: " + to_string(index));

    if (hEvents1[index] == svc.m_StopEvent)
    {
        pReceiver->ExitInstance();
        pProcess->ExitInstance();
        pSender->SignalStop();

        HANDLE hEvents3[3];
        hEvents3[0] = pSender->GetStoppedEvent();
        hEvents3[1] = pReceiver->GetStoppedEvent();
        hEvents3[2] = pProcess->GetStoppedEvent();

        WaitForMultipleObjects(3, hEvents3, TRUE, INFINITE) - WAIT_OBJECT_0;

        WSACleanup();
        svc.m_Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
        Sleep(100);
        return;
    }

    pReceiver->ResumeThread();
    pProcess->ResumeThread();

    HANDLE hEvents2[3];
    hEvents2[0] = pProcess->GetStartEvent();
    hEvents2[1] = pProcess->GetStartFailedEvent();
    hEvents2[2] = svc.m_StopEvent;

    index = WaitForMultipleObjects(3, hEvents2, FALSE, INFINITE) - WAIT_OBJECT_0;
    Log("Helper Startup Event Index: " + to_string(index));

    if (hEvents2[index] == svc.m_StopEvent)
    {
        pSender->SignalStop();
        pReceiver->SignalStop();
        pProcess->SignalStop();

        HANDLE hEvents3[3];
        hEvents3[0] = pSender->GetStoppedEvent();
        hEvents3[1] = pReceiver->GetStoppedEvent();
        hEvents3[2] = pProcess->GetStoppedEvent();

        WaitForMultipleObjects(3, hEvents3, TRUE, INFINITE) - WAIT_OBJECT_0;

        WSACleanup();
        svc.m_Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
        Sleep(100);
        return;
    }

    if (hEvents2[index] == pProcess->GetStartFailedEvent())
    {
        Log("Helper Process Startup failed");
        WaitForSingleObject(svc.m_StopEvent, INFINITE);
        pSender->SignalStop();
        pReceiver->SignalStop();
        pProcess->SignalStop();

        HANDLE hEvents3[3];
        hEvents3[0] = pSender->GetStoppedEvent();
        hEvents3[1] = pReceiver->GetStoppedEvent();
        hEvents3[2] = pProcess->GetStoppedEvent();

        WaitForMultipleObjects(3, hEvents3, TRUE, INFINITE) - WAIT_OBJECT_0;

        WSACleanup();
        svc.m_Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
        Sleep(100);
        return;
    }

    svc.WaitForHelper(hPipe);

    WaitForSingleObject(svc.m_StopEvent, INFINITE);
    pSender->SignalStop();
    pReceiver->SignalStop();
    pProcess->SignalStop();

    HANDLE hEvents3[3];
    hEvents3[0] = pSender->GetStoppedEvent();
    hEvents3[1] = pReceiver->GetStoppedEvent();
    hEvents3[2] = pProcess->GetStoppedEvent();

    WaitForMultipleObjects(3, hEvents3, TRUE, INFINITE) - WAIT_OBJECT_0;

    WSACleanup();
    svc.m_Status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(svc.m_StatusHandle, &svc.m_Status);
    Sleep(100);
}

HANDLE RemoteDesktopService::CreatePipe()
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);

    PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR)LocalAlloc(
        LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

    InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);

    SetSecurityDescriptorDacl(
        psd,
        TRUE,
        NULL,
        FALSE
    );

    sa.lpSecurityDescriptor = psd;
    sa.bInheritHandle = FALSE;

    HANDLE hPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\MyRemoteDesktopPipe",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        64 * 1024,
        64 * 1024,
        0,
        &sa
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::string strErr = to_string(error) + ": Error in pipe creation";
        Log(strErr);
    }
    else
        Log("Pipe Creation successfull");

    return hPipe;
}

BOOL RemoteDesktopService::WaitForHelper(HANDLE hPipe)
{
    Log("Waiting for helper to connect to pipe...");

    BOOL connected = ConnectNamedPipe(hPipe, nullptr);

    if (!connected)
    {
        DWORD err = GetLastError();

        if (err == ERROR_PIPE_CONNECTED)
        {
            Log("Helper connected (early connection)");
            return TRUE;
        }

        Log("ConnectNamedPipe failed");
        return FALSE;
    }

    Log("Helper connected to pipe");
    return TRUE;
}