#include "pch.h"
#include "RDCCommunicator.h"
#include "RDCLogger.h"
#include <string>
#include <algorithm>
using namespace std;

#define WM_NEW_FRAME (WM_USER + 100)

#pragma comment(lib, "Ws2_32.lib")

IMPLEMENT_DYNCREATE(RDCCommunicator, CWinThread)

RDCCommunicator::RDCCommunicator() : m_bRunning(FALSE), m_socket(INVALID_SOCKET) {}

BOOL RDCCommunicator::InitInstance()
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		Log("Error in openning socket");
		return FALSE;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;

	InetPton(AF_INET, L"127.0.0.1", &serverService.sin_addr.s_addr);
	serverService.sin_port = htons(6000);

	int result = connect(m_socket, (sockaddr*)&serverService, sizeof(serverService));
	if (result == SOCKET_ERROR)
	{
		Log("Connection Failed");
		return FALSE;
	}

	return TRUE;
}

int RDCCommunicator::Run()
{ 
    m_bRunning = TRUE;

    while (m_bRunning)
    {
        if (m_socket == INVALID_SOCKET)
        {
            Sleep(100);
            continue;
        }

        uint32_t frameSize;
        int bytesReceived = recv(m_socket, (char*)&frameSize, sizeof(frameSize), 0);

        if (bytesReceived == 0)
        {
            Log("Server disconnected");
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            continue;
        }
        else if (bytesReceived < 0)
        {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
            {
                Sleep(10);
                continue;
            }
            Log("Receive error: " + to_string(error));
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            continue;
        }
        else if (bytesReceived != sizeof(frameSize))
        {
            continue;
        }

        // Convert from network byte order
        frameSize = ntohl(frameSize);

        if (frameSize > 10 * 1024 * 1024) // Sanity check: max 10MB
        {
            Log("Invalid frame size: " + to_string(frameSize));
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            continue;
        }

        // 2. Receive frame data
        vector<BYTE> jpegData(frameSize);
        int totalReceived = 0;

        while (totalReceived < frameSize)
        {
            char buffer[1024];
            int remaining = frameSize - totalReceived;
            int chunkSize = min(remaining, sizeof(buffer));

            bytesReceived = recv(m_socket, buffer, chunkSize, 0);

            if (bytesReceived <= 0)
            {
                if (bytesReceived == 0)
                    Log("Server disconnected during frame transfer");
                else if (WSAGetLastError() != WSAEWOULDBLOCK)
                    Log("Frame receive error: " + to_string(WSAGetLastError()));

                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                break;
            }
            memcpy(jpegData.data() + totalReceived, buffer, bytesReceived);
            totalReceived += bytesReceived;
        }

        if (totalReceived == frameSize)
        {
            Log("Frame Received Successfully");

            if (m_pNotifyWnd)
            {
                m_pNotifyWnd->SetLastJpeg(jpegData);
                m_pNotifyWnd->PostMessage(WM_NEW_FRAME, NULL, NULL);
            }
        }
        Sleep(1);
    }
    return 0;
}

int RDCCommunicator::ExitInstance()
{
	if (m_socket != INVALID_SOCKET)
		closesocket(m_socket);
	return CWinThread::ExitInstance();
}
