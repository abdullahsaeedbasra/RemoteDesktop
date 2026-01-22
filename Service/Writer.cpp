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

	while (::WaitForSingleObject(m_hStop, 50) != WAIT_OBJECT_0)
	{
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(m_socket, &readSet);

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		int ready = select(0, &readSet, nullptr, nullptr, &timeout);
		if (ready > 0 && FD_ISSET(m_socket, &readSet))
		{
			char buffer[512];
			int bytes = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
			if (bytes == 0 || bytes == SOCKET_ERROR)
			{
				SetEvent(m_hClientDisconnected);
				::WaitForSingleObject(m_hStop, INFINITE);
				break;
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