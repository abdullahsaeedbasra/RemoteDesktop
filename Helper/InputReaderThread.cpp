#include "pch.h"
#include "InputReaderThread.h"
#include "ScreenCaptureThread.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(InputReaderThread, CWinThread)

InputReaderThread::InputReaderThread()
{
	m_hStopped = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

BOOL InputReaderThread::InitInstance()
{
	return TRUE;
}

int InputReaderThread::Run()
{
	OVERLAPPED olForPipe = {};
	olForPipe.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_bRunning = TRUE;
	while (m_bRunning)
	{
		BOOL bRetVal = FALSE;
		DWORD dwMsg = 0;
		DWORD dwBytesRead = 0;
		{
			Log("Reader: Inside Lock block");
			CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
			Log("Reader: After Lock");
			bRetVal = ReadFile(m_pNamedPipe->handle, &dwMsg, sizeof(dwMsg), &dwBytesRead, &olForPipe);
		}
		Log("REader: Waiting on overlap event");
		::WaitForSingleObject(olForPipe.hEvent, INFINITE);
		Log("REader: overlap event Signaled");

		bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);
		Log("Reader: After getting overlapped result");

		if (!bRetVal || dwBytesRead != sizeof(dwMsg))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_BROKEN_PIPE)
				Log("Pipe closed");
			else
				Log("Failed to read message" + to_string(dwError));
		}
		else
		{
			dwMsg = ntohl(dwMsg);
			if (dwMsg == -1)
			{
				Log("Client Disconnected. Closing process");
				m_pScreenThread->SignalStop();
				m_bRunning = FALSE;
			}
		}
	}
	SetEvent(m_hStopped);
	return 0;
}

void InputReaderThread::SetPipe(NamedPipe* pipe)
{
	m_pNamedPipe = pipe;
}

InputReaderThread::~InputReaderThread()
{

}