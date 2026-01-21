#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "Helper.h"
#include "Logger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHelperApp theApp;

BOOL CHelperApp::InitInstance()
{
	return TRUE;
}

int CHelperApp::Run()
{
    Log("Helper Started...");
    HANDLE hPipe = ConnectToServicePipe();
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        m_pScreenThread = (ScreenCaptureThread*)AfxBeginThread(RUNTIME_CLASS(ScreenCaptureThread), THREAD_PRIORITY_NORMAL, 0,
            CREATE_SUSPENDED);
        m_pInputReader = (InputReaderThread*)AfxBeginThread(RUNTIME_CLASS(InputReaderThread), THREAD_PRIORITY_NORMAL, 0,
            CREATE_SUSPENDED);
        m_pScreenThread->SetPipe(hPipe);
        m_pInputReader->SetPipe(hPipe);
        m_pInputReader->m_pScreenThread = m_pScreenThread;

        m_pScreenThread->ResumeThread();
        m_pInputReader->ResumeThread();

        HANDLE hEvents[2] = {
            m_pScreenThread->m_hStopped,
            m_pInputReader->m_hStopped
        };

        WaitForMultipleObjects(2, hEvents, TRUE, INFINITE);
        m_pScreenThread = nullptr;
    }

    m_pInputReader = nullptr;
    FlushFileBuffers(hPipe);
    CloseHandle(hPipe);
    hPipe = INVALID_HANDLE_VALUE;

    Log("Exiting");
    return 0;
}

HANDLE CHelperApp::ConnectToServicePipe()
{
    Log("Inside ConnectToServerPipe");

    const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\MyRemoteDesktopPipe";

    HANDLE hPipe = CreateFileW(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        Log("Connected to service pipe");
        return hPipe;
    }
    Log("Invalid Handle");
    return INVALID_HANDLE_VALUE;
}
