#include "Writer.h"
#include "Logger.h"
#include "Message.h"

IMPLEMENT_DYNCREATE(Writer, CWinThread)

Writer::Writer() : m_socket(INVALID_SOCKET), m_bRunning(FALSE), m_hSocketEvent(INVALID_HANDLE_VALUE)
{ 
	Log("Writer Constructor()");
	m_hStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hStopped = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hClientDisconnected = CreateEvent(nullptr, FALSE, FALSE, L"ClientDisconnected");
	m_hStopWhenClientIsConnected = CreateEvent(NULL, TRUE, FALSE, NULL);
}

BOOL Writer::InitInstance()
{
	Log("Writer Thread: InitInstance()");
	return TRUE;
}

int Writer::Run()
{
	Log("Writer Thread: Run()");

	BOOL bRetVal = FALSE;
	DWORD dwBytesWritten = 0;
	OVERLAPPED olPipe = {};
	olPipe.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	HANDLE arrHandles[3] = { m_hStop, m_hSocketEvent, m_hStopWhenClientIsConnected };
	m_bRunning = TRUE;
	while (m_bRunning)
	{
		int iIndex = ::WaitForMultipleObjects(3, arrHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
		if (iIndex == 0)
		{
			Log("Writer: Stop Signaled");
			m_bRunning = FALSE;
		}
		else if (iIndex == 1)
		{
			WSANETWORKEVENTS ne{};
			WSAEnumNetworkEvents(m_socket, m_hSocketEvent, &ne);

			if (ne.lNetworkEvents & FD_READ)
			{
				BOOL bRunning = TRUE;
				while (bRunning)
				{
					MessageHeader hdr{};
					int peak = recv(m_socket, (char*)&hdr, sizeof(hdr), MSG_PEEK);

					if (peak == SOCKET_ERROR) {
						int err = WSAGetLastError();
						if (err == WSAEWOULDBLOCK) break;

						break;
					}
					if (peak < sizeof(hdr)) break;

					if (!recvAll(m_socket, (char*)&hdr, sizeof(hdr))) break;

					hdr.type = ntohl(hdr.type);
					hdr.size = ntohl(hdr.size);

					switch (hdr.type)
					{
					case MSG_MOUSEMOVE:
					{
						if (hdr.size != sizeof(MouseMoveData))
						{
							Log("hdr.size != sizeof(MouseMoveData)");
							bRunning = FALSE;
							continue;
						}

						MouseMoveData data{};
						if (!recvAll(m_socket, (char*)&data, sizeof(data)))
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								Log("Error in Receiving Mouse Move Data");
							}
							bRunning = FALSE;
							continue;
						}

						Log("Received Mouse Move Position percentage successfully");
						hdr.type = htonl(hdr.type);
						hdr.size = htonl(hdr.size);

						{
							CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
							WriteFile(m_pNamedPipe->handle, &hdr, sizeof(hdr), &dwBytesWritten, &olPipe);
						}

						::WaitForSingleObject(olPipe.hEvent, INFINITE);
						bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olPipe, &dwBytesWritten, FALSE);

						if (!bRetVal)
						{
							Log("Failed to write mouse move message header");
							bRunning = FALSE;
							continue;
						}

						bRetVal = FALSE;
						dwBytesWritten = 0;

						{
							CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
							WriteFile(m_pNamedPipe->handle, &data, sizeof(data), &dwBytesWritten, &olPipe);
						}

						::WaitForSingleObject(olPipe.hEvent, INFINITE);
						bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olPipe, &dwBytesWritten, FALSE);

						if (!bRetVal)
						{
							Log("Failed to write mouse move data");
							bRunning = FALSE;
							continue;
						}
						Log("SuccessFully Written Mouse Data on Pipe");
					}
					break;

					case MSG_LEFT_BUTTON_DOWN:
					case MSG_LEFT_BUTTON_UP:
					case MSG_RIGHT_BUTTON_DOWN:
					case MSG_RIGHT_BUTTON_UP:
					case MSG_LEFT_BUTTON_DOUBLE_CLICK:
					case MSG_MOUSE_WHEEL_UP:
					case MSG_MOUSE_WHEEL_DOWN:
					{
						if (hdr.size != sizeof(MessageHeader))
						{
							Log("hdr.size != sizeof(MessageHeader) in mouse button");
							bRunning = FALSE;
							continue;
						}
						hdr.type = htonl(hdr.type);
						hdr.size = htonl(hdr.size);

						bRetVal = FALSE;
						dwBytesWritten = 0;
						{
							CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
							WriteFile(m_pNamedPipe->handle, &hdr, sizeof(hdr), &dwBytesWritten, &olPipe);
						}

						::WaitForSingleObject(olPipe.hEvent, INFINITE);
						bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olPipe, &dwBytesWritten, FALSE);

						if (!bRetVal)
						{
							Log("Failed to write mouse button message header");
							bRunning = FALSE;
							continue;
						}
					}
					break;

					case MSG_KEYBOARD:
					{
						if (hdr.size != sizeof(KeyData))
						{
							Log("hdr.size != sizeof(KeyData) in KeyData");
							bRunning = FALSE;
							continue;
						}
						Log("KeyBoard Received ");
						hdr.type = htonl(hdr.type);
						hdr.size = htonl(hdr.size);

						bRetVal = FALSE;
						dwBytesWritten = 0;
						{
							CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
							WriteFile(m_pNamedPipe->handle, &hdr, sizeof(hdr), &dwBytesWritten, &olPipe);
						}

						::WaitForSingleObject(olPipe.hEvent, INFINITE);
						bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olPipe, &dwBytesWritten, FALSE);

						if (!bRetVal)
						{
							Log("Failed to write KEYBOARD message header");
							bRunning = FALSE;
							continue;
						}

						KeyData keyData{};
						if (!recvAll(m_socket, (char*)&keyData, sizeof(keyData)))
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								Log("Error in Receiving KEY DATA");
								bRunning = FALSE;
								continue;
							}
						}

						Log("KEY DATA RECEIVED SUCCESSFULLY");
						bRetVal = FALSE;
						dwBytesWritten = 0;
						{
							CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
							WriteFile(m_pNamedPipe->handle, &keyData, sizeof(keyData), &dwBytesWritten, &olPipe);
						}

						::WaitForSingleObject(olPipe.hEvent, INFINITE);
						bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olPipe, &dwBytesWritten, FALSE);

						if (!bRetVal)
						{
							Log("Failed to write Key Data on pipe");
							bRunning = FALSE;
							continue;
						}
						Log("KEY DATA WRITTEN ON PIPE SUCCESSFULLY");
					}
					break;

					default:
						break;
					}
				}
			}
			else if (ne.lNetworkEvents & FD_CLOSE)
			{
				CloseHelper();
				SetEvent(m_hClientDisconnected);
				::WaitForSingleObject(m_hStop, INFINITE);
				Log("Writer Stop Wait Released");
				m_bRunning = FALSE;
			}
		}
		else if (iIndex == 2)
		{
			CloseHelper();
			m_bRunning = FALSE;
		}
	}

	Log("Writer: Exiting");
	SetEvent(m_hStopped);
	return 0;
}

bool Writer::recvAll(SOCKET s, char* buffer, int totalBytes)
{
	int bytesReceived = 0;

	while (bytesReceived < totalBytes)
	{
		int ret = recv(s, buffer + bytesReceived, totalBytes - bytesReceived, 0);

		if (ret <= 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			return false;
		}

		bytesReceived += ret;
	}

	return true;
}

bool Writer::CloseHelper()
{
	OVERLAPPED olForPipe = {};
	olForPipe.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (olForPipe.hEvent == INVALID_HANDLE_VALUE)
	{
		Log("Failed to create overlap event");
		return false;
	}

	MessageHeader hdr{};
	hdr.type = htonl(MSG_CLIENT_DISCONNECTED);
	hdr.size = htonl(sizeof(hdr));

	DWORD dwBytesWritten = 0;
	BOOL bRetVal = FALSE;
	{
		CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
		WriteFile(m_pNamedPipe->handle, &hdr, sizeof(hdr), &dwBytesWritten, &olForPipe);
	}
	::WaitForSingleObject(olForPipe.hEvent, INFINITE);
	bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesWritten, FALSE);

	if (!bRetVal)
	{
		Log("Failed to write disconnection message");
		return false;
	}

	Log("Disconnection Message Written on Pipe");
	return true;
}

void Writer::SetPipe(NamedPipe* pipe)
{
	m_pNamedPipe = pipe;
}

void Writer::SignalStop()
{
	Log("Writer: Stop Signaled");
	SetEvent(m_hStop);
}

void Writer::SignalStopWhenClientIsConnected()
{
	Log("Writer Stop Signaled When Client Is Connected");
	SetEvent(m_hStopWhenClientIsConnected);
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
	m_bRunning = FALSE;
	if (m_hStop != INVALID_HANDLE_VALUE)
		CloseHandle(m_hStop);
	if (m_hStopped != INVALID_HANDLE_VALUE)
		CloseHandle(m_hStopped);
	if (m_hSocketEvent != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSocketEvent);
	if (m_hStopWhenClientIsConnected != INVALID_HANDLE_VALUE)
		CloseHandle(m_hStopWhenClientIsConnected);
	Log("Writer Destructor End");
}