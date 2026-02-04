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

	InetPton(AF_INET, L"192.168.0.106", &serverService.sin_addr.s_addr);
	serverService.sin_port = htons(6000);

	int result = connect(m_socket, (sockaddr*)&serverService, sizeof(serverService));
	if (result == SOCKET_ERROR)
	{
		Log("Connection Failed");
		return FALSE;
	}

    BOOL bOptVal = TRUE;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&bOptVal, sizeof(BOOL));

	return TRUE;
}

int RDCCommunicator::Run()
{ 
    m_bRunning = TRUE;

    while (m_bRunning)
    {
        uint32_t frameSize;
        if (!recvAll(m_socket, (char*)&frameSize, sizeof(frameSize)))
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                Log("FrameSize Receive Error");
            }
            continue;
        }

        frameSize = ntohl(frameSize);

        if (frameSize > 10 * 1024 * 1024)
        {
            Log("Invalid frame size");
            continue;
        }

        vector<BYTE> jpegData(frameSize);

        if (!recvAll(m_socket, (char*)jpegData.data(), frameSize))
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                Log("Jpeg Receive Error");
            }
            continue;
        }

        Log("Frame Received Successfully");

        if (m_pNotifyWnd)
        {
            m_pNotifyWnd->SetLastJpeg(jpegData);
            m_pNotifyWnd->PostMessage(WM_NEW_FRAME, NULL, NULL);
        }
        Sleep(1);
    }
    return 0;
}

void RDCCommunicator::SendMouseMove(int percentage_x, int percentage_y)
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_MOUSEMOVE);
    hdr.size = htonl(sizeof(MouseMoveData));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending mouse header");
        return;
    }

    MouseMoveData data{};
    data.percentage_x = htonl(percentage_x);
    data.percentage_y = htonl(percentage_y);

    bSuccess = sendAll(m_socket, (char*)&data, sizeof(data));
    if (!bSuccess)
    {
        Log("Error in sending mouse position");
    }
}

void RDCCommunicator::SendLeftButtonDown()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_LEFT_BUTTON_DOWN);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Left mouse button down");
    }
}

void RDCCommunicator::SendLeftButtonUp()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_LEFT_BUTTON_UP);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Left mouse button down");
    }
}

void RDCCommunicator::SendRightButtonDown()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_RIGHT_BUTTON_DOWN);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Right mouse button down");
    }
}

void RDCCommunicator::SendRightButtonUp()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_RIGHT_BUTTON_UP);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Right mouse button up");
    }
}

void RDCCommunicator::SendLeftButtonDoubleClick()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_LEFT_BUTTON_DOUBLE_CLICK);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Left Button Double Click");
    }
}

void RDCCommunicator::SendMouseWheelUp()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_MOUSE_WHEEL_UP);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Mouse Wheel Up");
    }
}

void RDCCommunicator::SendMouseWheelDown()
{
    MessageHeader hdr{};
    hdr.type = htonl(MSG_MOUSE_WHEEL_DOWN);
    hdr.size = htonl(sizeof(MessageHeader));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Mouse Wheel Down");
    }
}

void RDCCommunicator::SendKeyDown(KeyData* pData)
{
    Log("SEND KEY DOWN CALLED");
    MessageHeader hdr{};
    hdr.type = htonl(MSG_KEYBOARD);
    hdr.size = htonl(sizeof(KeyData));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Key down header");
    }

    KeyData data = *pData;
    data.flags = htonl(data.flags);
    data.vk = htonl(data.vk);
    bSuccess = sendAll(m_socket, (char*)&data, sizeof(data));

    if (!bSuccess)
    {
        Log("Error in sending Key Down Data");
    }
    delete pData;
}

void RDCCommunicator::SendKeyUp(KeyData* pData)
{
    Log("SEND KEY UP CALLED");
    MessageHeader hdr{};
    hdr.type = htonl(MSG_KEYBOARD);
    hdr.size = htonl(sizeof(KeyData));

    bool bSuccess = sendAll(m_socket, (char*)&hdr, sizeof(hdr));
    if (!bSuccess)
    {
        Log("Error in sending Key Up Header");
        return;
    }

    KeyData data = *pData;
    data.flags = htonl(data.flags);
    data.vk = htonl(data.vk);
    bSuccess = sendAll(m_socket, (char*)&data, sizeof(data));

    if (!bSuccess)
    {
        Log("Error in sending Key Up Data");
    }
    delete pData;
}

bool RDCCommunicator::sendAll(SOCKET socket, char* buffer, int totalBytes)
{
    int bytesSent = 0;

    while (bytesSent < totalBytes)
    {
        int sent = send(socket, buffer + bytesSent, totalBytes - bytesSent, 0);

        if (sent <= SOCKET_ERROR)
            return false;

        bytesSent += sent;
    }
    return true;
}

bool RDCCommunicator::recvAll(SOCKET s, char* buffer, int totalBytes)
{
    int received = 0;

    while (received < totalBytes)
    {
        int ret = recv(s, buffer + received, totalBytes - received, 0);

        if (ret <= 0)
            return false;

        received += ret;
    }

    return true;
}

int RDCCommunicator::ExitInstance()
{
	if (m_socket != INVALID_SOCKET)
		closesocket(m_socket);
	return CWinThread::ExitInstance();
}
