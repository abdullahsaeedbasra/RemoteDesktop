#include "Listener.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(Listener, CWinThread)

Listener::Listener() : m_listeningSocket(INVALID_SOCKET), m_socket(INVALID_SOCKET),
						m_hSocketEvent(INVALID_HANDLE_VALUE)
{
	Log("Listener Constructor()");
	m_hStop = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_hStopped = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

BOOL Listener::InitInstance()
{
	Log("Listener Thread: InitInstance()");

	m_hClientConnectedEvent = CreateEvent(nullptr, FALSE, FALSE, L"ClientConnected");
	m_hClientDisconnectedEvent = CreateEvent(nullptr, FALSE, FALSE, L"ClientDisconnectedListener");

	if (!m_hClientConnectedEvent || !m_hClientDisconnectedEvent)
	{
		Log("Listener: Failed to open events, error: " + std::to_string(GetLastError()));
		return FALSE;
	}

	m_listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listeningSocket == INVALID_SOCKET)
	{
		Log("Listener Thread: Failed to create a server socket");
		return FALSE;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(6000);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(m_listeningSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		Log("Listener Thread: Bind Failed");
		closesocket(m_listeningSocket);
		return FALSE;
	}

	if (listen(m_listeningSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		Log("Listener Thread: Listen Failed");
		closesocket(m_listeningSocket);
		return FALSE;
	}

	Log("Listener Thread: Server Listening on port 6000");

	m_hSocketEvent = WSACreateEvent();
	if (m_hSocketEvent == WSA_INVALID_EVENT)
	{
		Log("Listener: WSACreateEvent failed");
		return FALSE;
	}

	if (WSAEventSelect(
		m_listeningSocket,
		m_hSocketEvent,
		FD_ACCEPT | FD_CLOSE
	) == SOCKET_ERROR)
	{
		Log("Listener: WSAEventSelect failed");
		return FALSE;
	}

	return TRUE;
}

int Listener::Run()
{
	Log("Listener Run()");

	HANDLE hEvents[3];
	hEvents[0] = m_hStop;
	hEvents[1] = m_hSocketEvent;
	hEvents[2] = m_hClientDisconnectedEvent;

	while (true)
	{
		DWORD wait = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);

		DWORD index = wait - WAIT_OBJECT_0;

		if (index == 0)
		{
			Log("Listener: Stop signaled");
			break;
		}

		if (index == 1)
		{
			WSANETWORKEVENTS ne{};
			WSAEnumNetworkEvents(m_listeningSocket, m_hSocketEvent, &ne);

			if (ne.lNetworkEvents & FD_ACCEPT)
			{
				SOCKET clientSocket = accept(m_listeningSocket, nullptr, nullptr);
				if (clientSocket != INVALID_SOCKET)
				{
					m_socket = clientSocket;
					Log("Listener: Client accepted");
					SetEvent(m_hClientConnectedEvent);
				}
			}

			if (ne.lNetworkEvents & FD_CLOSE)
			{
				Log("Listener: Listening socket closed");
				closesocket(m_listeningSocket);
				m_listeningSocket = INVALID_SOCKET;
				closesocket(m_socket);
				m_socket = INVALID_SOCKET;
				break;
			}
		}

		if (index == 2)
		{
			Log("Listener: Client disconnected, closing socket");

			if (m_socket != INVALID_SOCKET)
			{
				closesocket(m_socket);
				m_socket = INVALID_SOCKET;
			}

			ResetEvent(m_hClientDisconnectedEvent);
		}
	}

	SetEvent(m_hStopped);
	Log("Listener thread exiting");
	return 0;
}


SOCKET Listener::GetSocket() const
{
	return m_socket;
}

void Listener::SignalStop()
{
	SetEvent(m_hStop);
}

Listener::~Listener()
{
	Log("Listener Destructor()");
	if (m_hStop != nullptr)
		CloseHandle(m_hStop);
	if (m_hStopped != nullptr)
		CloseHandle(m_hStopped);
	Log("Listener Destructor End");
}