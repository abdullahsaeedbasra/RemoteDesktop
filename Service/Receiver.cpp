#include "Receiver.h"
#include "Logger.h"

#define CLIENT_DISCONNECTED '-999'

IMPLEMENT_DYNCREATE(Receiver, CWinThread)

Receiver::Receiver() : m_bRunning(FALSE), m_clientSocket(INVALID_SOCKET), m_hPipe(INVALID_HANDLE_VALUE),
					   m_hStopEvent(INVALID_HANDLE_VALUE), m_hStoppedEvent(INVALID_HANDLE_VALUE)
{ }

BOOL Receiver::InitInstance()
{
	return TRUE;
}

int Receiver::Run()
{
	m_bRunning = TRUE;
	while (m_bRunning)
	{
		if (WaitForSingleObject(m_hStopEvent, 10) == WAIT_OBJECT_0)
			return 0;

		Sleep(100);
	}
	return 0;
}

void Receiver::SetClientSocket(const SOCKET& socket)
{
	m_clientSocket = socket;
}

void Receiver::SetPipe(const HANDLE& hPipe)
{
	m_hPipe = hPipe;
}

void Receiver::CreateStopEvent()
{
	m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, L"StopEvent");
}

void Receiver::CreateStoppedEvent()
{
	m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, L"StopEvent");
}

void Receiver::SignalStop()
{
	SetEvent(m_hStopEvent);
}

HANDLE Receiver::GetStoppedEvent() const
{
	return m_hStoppedEvent;
}

int Receiver::ExitInstance()
{
	SetEvent(m_hStoppedEvent);
	return CWinThread::ExitInstance();
}

Receiver::~Receiver()
{
	m_bRunning = FALSE;
}