#include "ProcessManager.h"
#include "Logger.h"
#include "Reader.h"
#include "Writer.h"

#include <wtsapi32.h>
#include <userenv.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "advapi32.lib")

IMPLEMENT_DYNCREATE(ProcessManager, CWinThread)

ProcessManager::ProcessManager() : m_socket(INVALID_SOCKET), m_hHelperProcess(INVALID_HANDLE_VALUE)
{
    Log("Process Manager Constructor()");

    m_hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hStopped = CreateEvent(NULL, TRUE, FALSE, NULL);

    m_pNamedPipe = new NamedPipe();
}

BOOL ProcessManager::InitInstance()
{
    Log("Process Manager: InitInstance()");
	return TRUE;
}

int ProcessManager::Run()
{
    Log("Process Manager Run()");

    CreatePipe();
    if (m_pNamedPipe->handle == INVALID_HANDLE_VALUE)
    {
        SetEvent(m_hStopped);
        return -1;
    }

    if (!StartHelper())
    {
        Log("Error in starting helper");
        SetEvent(m_hStopped);
        return -1;
    }

    if (!WaitForHelper())
    {
        SetEvent(m_hStopped);
        return -1;
    }

    m_pReader = (Reader*)AfxBeginThread(RUNTIME_CLASS(Reader), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    m_pWriter = (Writer*)AfxBeginThread(RUNTIME_CLASS(Writer), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

    if (!m_pReader || !m_pWriter)
    {
        Log("Error in starting reader or writer");
        SetEvent(m_hStopped);
        return -1;
    }

    m_pReader->SetPipe(m_pNamedPipe);
    m_pReader->SetSocket(m_socket);
    m_pWriter->SetPipe(m_pNamedPipe);
    m_pWriter->SetSocket(m_socket);
    m_pReader->ResumeThread();
    m_pWriter->ResumeThread();


    HANDLE arrHandles[3] = { m_hStop, m_pReader->m_hStopped, m_hHelperProcess };
    int iIndex = ::WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
    if (iIndex == 1)
    {
        m_pReader->SignalStop();
        ::WaitForSingleObject(m_pReader->m_hStopped, INFINITE);
    }

    FlushFileBuffers(m_pNamedPipe->handle);
    DisconnectNamedPipe(m_pNamedPipe->handle);
    CloseHandle(m_pNamedPipe->handle);
    m_pNamedPipe->handle = INVALID_HANDLE_VALUE;

    CloseHandle(m_hHelperProcess);
    m_hHelperProcess = INVALID_HANDLE_VALUE;

    SetEvent(m_hStopped);
    return 0;
}

void ProcessManager::SignalStop()
{
    SetEvent(m_hStop);
}

void ProcessManager::SetSocket(const SOCKET& socket)
{
    m_socket = socket;
}

ProcessManager::~ProcessManager()
{
    Log("Process Manager Destructor()");
    if (m_hStop != nullptr)
        CloseHandle(m_hStop);
    if (m_hStopped != nullptr)
        CloseHandle(m_hStopped);
    if (m_pNamedPipe)
    {
        delete m_pNamedPipe;
        m_pNamedPipe = nullptr;
    }

    Log("Process Manager Destructor Ended");
}

BOOL ProcessManager::StartHelper()
{
    Log("Inside StartHelper");
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
        L"C:\\Projects\\RemoteDesktop\\Helper\\x64\\Debug\\Helper.exe",
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        env,
        L"C:\\Projects\\RemoteDesktop\\Helper",
        &si,
        &pi
    );

    DestroyEnvironmentBlock(env);
    CloseHandle(hPrimaryToken);

    if (!ok)
        return false;

    m_hHelperProcess = pi.hProcess;
    CloseHandle(pi.hThread);

    return true;
}

void ProcessManager::CreatePipe()
{
    Log("Inside CreatePipe()");
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
    m_pNamedPipe->handle = CreateNamedPipeW(
        L"\\\\.\\pipe\\MyRemoteDesktopPipe",
        PIPE_ACCESS_DUPLEX | 
        FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | 
        PIPE_READMODE_MESSAGE | 
        PIPE_WAIT,
        1,
        64 * 1024,
        64 * 1024,
        0,
        &sa
    );

    if (m_pNamedPipe->handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::string strErr = to_string(error) + ": Error in pipe creation";
        Log(strErr);
    }
    else
        Log("Pipe Creation successfull");
}

BOOL ProcessManager::WaitForHelper()
{
    Log("Waiting for helper to connect to pipe...");

    BOOL connected = ConnectNamedPipe(m_pNamedPipe->handle, nullptr);

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