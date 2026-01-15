#define _HAS_STD_BYTE 0

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOBYTE
#define _WINSOCKAPI_

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afxwin.h>
#include <windows.h>
#include "Service.h"
#include "Logger.h"
#include <conio.h>
#include <wtsapi32.h>
#include <userenv.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "advapi32.lib")

void RunAsConsole();

int main(int argc, TCHAR* argv[]) {
#ifdef _DEBUG
    Log("Entered Debug Only Area");
    RunAsConsole();
#else
    // Release-only code
    RemoteDesktopService::Instance().Run();
    return 0;
#endif
}

HANDLE CreatePipe()
{
    HANDLE hPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\MyRemoteDesktopPipe",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE |
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,
        1,
        64 * 1024,
        64 * 1024,
        0,
        NULL
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

BOOL WaitForHelper(HANDLE hPipe)
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

BOOL StartHelper()
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
        L"D:\\Visual C++\\Helper\\x64\\Debug\\Helper.exe",
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        env,
        nullptr,
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

BOOL StartHelper_Console()
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        L"D:\\Visual C++\\Helper\\x64\\Debug\\Helper.exe",
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!ok)
    {
        Log("CreateProcessW failed");
        return FALSE;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

void RunAsConsole()
{
    HANDLE hPipe = CreatePipe();

    if (StartHelper_Console())
        Log("Process Creation Successful");
    else
        Log("Process Creation Failed");

    WaitForHelper(hPipe);
}
