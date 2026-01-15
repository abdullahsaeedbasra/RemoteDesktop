#include "ProcessManager.h"
#include "Logger.h"

#include <wtsapi32.h>
#include <userenv.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "advapi32.lib")

IMPLEMENT_DYNCREATE(ProcessManager, CWinThread)

ProcessManager::ProcessManager() : m_hStartEvent(INVALID_HANDLE_VALUE), m_hStartFailedEvent(INVALID_HANDLE_VALUE),
                                   m_hStopEvent(INVALID_HANDLE_VALUE), m_hStoppedEvent(INVALID_HANDLE_VALUE)
{ }

BOOL ProcessManager::InitInstance()
{
	return TRUE;
}

int ProcessManager::Run()
{
    if (StartHelper())
        SetEvent(m_hStartEvent);
    else
        SetEvent(m_hStartFailedEvent);

    WaitForSingleObject(m_hStopEvent, INFINITE);

	return 0;
}

BOOL ProcessManager::StartHelper()
{
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF)
    {
        Log("Error in getting session");
        return FALSE;
    }

    HANDLE hImpersonationToken = nullptr;
    if (!WTSQueryUserToken(sessionId, &hImpersonationToken))
        return false;

    HANDLE hPrimaryToken = nullptr;
    if (!DuplicateTokenEx(
        hImpersonationToken,
        TOKEN_ALL_ACCESS,
        nullptr,
        SecurityImpersonation,
        TokenPrimary,
        &hPrimaryToken))
    {
        CloseHandle(hImpersonationToken);
        return false;
    }

    CloseHandle(hImpersonationToken);

    LPVOID env = nullptr;
    if (!CreateEnvironmentBlock(&env, hPrimaryToken, FALSE))
    {
        CloseHandle(hPrimaryToken);
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.lpDesktop = (LPWSTR)L"winsta0\\default";

    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessAsUserW(
        hPrimaryToken,
        L"D:\\Visual C++\\RemoteDesktop\\Helper\\x64\\Debug\\Helper.exe",
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        env,
        L"D:\\Visual C++\\RemoteDesktop\\Helper",
        &si,
        &pi
    );

    DestroyEnvironmentBlock(env);
    CloseHandle(hPrimaryToken);

    if (!ok)
        return false;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

void ProcessManager::CreateStartEvent()
{
    m_hStartEvent = CreateEvent(NULL, TRUE, FALSE, L"Helper Startup Event");
}

void ProcessManager::CreateStartFailedEvent()
{
    m_hStartFailedEvent = CreateEvent(NULL, TRUE, FALSE, L"Helper Startup Failed Event");
}

HANDLE ProcessManager::GetStartEvent() const
{
    return m_hStartEvent;
}

HANDLE ProcessManager::GetStartFailedEvent() const
{
    return m_hStartFailedEvent;
}

void ProcessManager::CreateStopEvent()
{
    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, L"StopEvent");
}

void ProcessManager::CreateStoppedEvent()
{
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, L"StopEvent");
}

void ProcessManager::SignalStop()
{
    SetEvent(m_hStopEvent);
}

HANDLE ProcessManager::GetStoppedEvent() const
{
    return m_hStoppedEvent;
}

int ProcessManager::ExitInstance()
{
    SetEvent(m_hStoppedEvent);
	return CWinThread::ExitInstance();
}

ProcessManager::~ProcessManager()
{
}