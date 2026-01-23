#include "Writer.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(Writer, CWinThread)

Writer::Writer() : m_socket(INVALID_SOCKET)
{ 
	Log("Writer Constructor()");
	m_hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hStopped = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hClientDisconnected = CreateEvent(nullptr, FALSE, FALSE, L"ClientDisconnected");
}

BOOL Writer::InitInstance()
{
	Log("Writer Thread: InitInstance()");
	return TRUE;
}

int Writer::Run()
{
	Log("Writer Thread: Run()");

	HANDLE arrHandles[2] = { m_hStop, m_hSocketEvent };

	m_bRunning = TRUE;
	while (m_bRunning)
	{
		int iIndex = ::WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
		if (iIndex == 0)
		{

		}
		else if (iIndex == 1)
		{
			WSANETWORKEVENTS ne{};
			WSAEnumNetworkEvents(m_socket, m_hSocketEvent, &ne);

			if (ne.lNetworkEvents & FD_READ)
			{
				
			}
			else if (ne.lNetworkEvents & FD_CLOSE)
			{
				OVERLAPPED olForPipe = {};
				olForPipe.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				DWORD dwMsg = -1;
				dwMsg = htonl(dwMsg);
				DWORD dwBytesWritten = 0;
				BOOL bRetVal = FALSE;
				{
					CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
					bRetVal = WriteFile(m_pNamedPipe->handle, &dwMsg, sizeof(dwMsg), &dwBytesWritten, &olForPipe);
				}
				::WaitForSingleObject(olForPipe.hEvent, INFINITE);
				bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesWritten, FALSE);

				if (!bRetVal)
				{
					Log("Failed to write screen header");
				}

				SetEvent(m_hClientDisconnected);
				::WaitForSingleObject(m_hStop, INFINITE);
				m_bRunning = FALSE;
			}
		}
	}

	SetEvent(m_hStopped);
	return 0;
}

void Writer::SetPipe(NamedPipe* pipe)
{
	m_pNamedPipe = pipe;
}

void Writer::SignalStop()
{
	SetEvent(m_hStop);
}

void Writer::SetSocket(const SOCKET& socket)
{
	m_socket = socket;

	m_hSocketEvent = WSACreateEvent();
	if (m_hSocketEvent == WSA_INVALID_EVENT)
	{
		Log("Writer: WSACreateEvent failed");
		return;
	}

	if (WSAEventSelect(m_socket, m_hSocketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		Log("Writer: WSAEventSelect failed");
		return;
	}
}

Writer::~Writer()
{
	Log("Writer Destructor()");
	if (m_hStop != INVALID_HANDLE_VALUE)
		CloseHandle(m_hStop);
	if (m_hStopped != INVALID_HANDLE_VALUE)
		CloseHandle(m_hStopped);
	Log("Writer Destructor End");
}