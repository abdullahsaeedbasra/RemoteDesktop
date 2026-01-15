#include "Sender.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(Sender, CWinThread)

Sender::Sender() : m_bRunning(FALSE), m_hServerSocket(INVALID_SOCKET), 
                   m_clientSocket(INVALID_SOCKET),
                   m_hPipe(INVALID_HANDLE_VALUE),
				   m_hClientEvent(INVALID_HANDLE_VALUE),
				   m_hStopEvent(INVALID_HANDLE_VALUE),
				   m_hStoppedEvent(INVALID_HANDLE_VALUE)
{ }

BOOL Sender::InitInstance()
{ 
	m_hServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_hServerSocket == INVALID_SOCKET)
	{
		Log("Failed to create a server socket");
		return FALSE;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(6000);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(m_hServerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		Log("Bind Failed");
		closesocket(m_hServerSocket);
		return FALSE;
	}

	if (listen(m_hServerSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		Log("Listen Failed");
		closesocket(m_hServerSocket);
		return FALSE;
	}

	Log("Server Listening on port 6000");

	return TRUE;
}

int Sender::Run()
{
	Log("Sender's Run");

	sockaddr_in clientAddr;
	int clientSize = sizeof(clientAddr);

	while (true)
	{
		m_clientSocket = accept(m_hServerSocket, (sockaddr*)&clientAddr, &clientSize);
		if (m_clientSocket == INVALID_SOCKET)
			Log("Accept Failed");
		else
			break;
		Sleep(10);
	}

	if (!SetEvent(m_hClientEvent))
	{
		Log("Set Client Event Failed");
		return -1;
	}

	m_bRunning = TRUE;
	while (m_bRunning)
	{
		if (WaitForSingleObject(m_hStopEvent, 0) == WAIT_OBJECT_0)
			return 0;
		
        SendScreen();
	}
	return 0;
}

void Sender::SendScreen()
{
    while (m_bRunning)
    {
        if (WaitForSingleObject(m_hStopEvent, 0) == WAIT_OBJECT_0)
            return;

        uint32_t frameSizeNet = 0;
        DWORD read = 0;

        BOOL ok = ReadFile(
            m_hPipe,
            &frameSizeNet,
            sizeof(frameSizeNet),
            &read,
            NULL
        );

        if (!ok || read != sizeof(frameSizeNet))
        {
            Log("Failed to read frame size from pipe");
            return;
        }

        int sent = send(m_clientSocket,
            (char*)&frameSizeNet,
            sizeof(frameSizeNet),
            0);

        if (sent != sizeof(frameSizeNet))
        {
            Log("Failed to send frame size");
            return;
        }

        uint32_t frameSize = ntohl(frameSizeNet);

        DWORD remaining = frameSize;

        while (remaining > 0)
        {
            BYTE buffer[1024];
            DWORD toRead = min(remaining, (DWORD)sizeof(buffer));

            ok = ReadFile(
                m_hPipe,
                buffer,
                toRead,
                &read,
                NULL
            );

            if (!ok || read == 0)
            {
                Log("Pipe read failed while streaming JPEG");
                return;
            }

            DWORD offset = 0;
            while (offset < read)
            {
                sent = send(
                    m_clientSocket,
                    (char*)buffer + offset,
                    read - offset,
                    0
                );

                if (sent == SOCKET_ERROR)
                {
                    Log("Socket send failed");
                    return;
                }

                offset += sent;
            }

            remaining -= read;
        }

        Log("Frame forwarded (streamed)");
    }
}

void Sender::SetPipe(const HANDLE& hPipe)
{
	m_hPipe = hPipe;
}

void Sender::CreateClientEvent()
{
	m_hClientEvent = CreateEvent(NULL, TRUE, FALSE, L"ClientEvent");
}

void Sender::CreateStopEvent()
{
	m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, L"StopEvent");
}

void Sender::CreateStoppedEvent()
{
	m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, L"StopEvent");
}

void Sender::SignalStop()
{
	SetEvent(m_hStopEvent);
}

HANDLE Sender::GetStoppedEvent() const
{
	return m_hStoppedEvent;
}

HANDLE Sender::GetClientEvent() const
{
	return m_hClientEvent;
}

SOCKET Sender::GetCLientSocket() const
{
	return m_clientSocket;
}

int Sender::ExitInstance()
{
	SetEvent(m_hStoppedEvent);
	return CWinThread::ExitInstance();
}

Sender::~Sender()
{
	m_bRunning = FALSE;
}