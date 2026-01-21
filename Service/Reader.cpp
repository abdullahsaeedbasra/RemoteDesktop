#pragma once
#include "Reader.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(Reader, CWinThread)

enum MessageType : uint32_t
{
    MSG_SCREEN = 1,
    MSG_INPUT = 2
};

#pragma pack(push, 1)
struct PipeMessageHeader
{
    uint32_t type;
    uint32_t size;
};
#pragma pack(pop)

Reader::Reader() : m_socket(INVALID_SOCKET)
{ 
    Log("Reader Constructor()");
    m_hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hStopped = CreateEvent(NULL, TRUE, FALSE, NULL);
}

BOOL Reader::InitInstance()
{ 
    Log("Reader Thread: InitInstance()");
	return TRUE;
}

int Reader::Run()
{
    Log("Reader Thread started");

    while (WaitForSingleObject(m_hStop, 0) != WAIT_OBJECT_0)
    {
        PipeMessageHeader header{};
        DWORD read = 0;

        BOOL ok = ReadFile(
            m_hPipe,
            &header,
            sizeof(header),
            &read,
            nullptr
        );

        if (!ok || read != sizeof(header))
        {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE)
                Log("Pipe closed by helper");
            else
                Log("Failed to read pipe header");

            break;
        }

        uint32_t size = ntohl(header.size);

        if (size == 0 || size > 10 * 1024 * 1024)
        {
            Log("Invalid message size");
            break;
        }

        std::vector<BYTE> payload(size);
        DWORD total = 0;

        while (total < size)
        {
            DWORD r = 0;
            ok = ReadFile(
                m_hPipe,
                payload.data() + total,
                size - total,
                &r,
                nullptr
            );

            if (!ok || r == 0)
            {
                Log("Pipe read failed during payload");
                goto exit;
            }

            total += r;
        }

        if (header.type == MSG_SCREEN)
        {
            SendFrameToSocket(payload);
        }
        else if (header.type == MSG_INPUT)
        {
            Log("Service received input message (ignored here)");
        }
    }

exit:
    SetEvent(m_hStopped);
    return 0;
}

void Reader::SendFrameToSocket(const std::vector<BYTE>& jpeg)
{
    uint32_t sizeNet = htonl((uint32_t)jpeg.size());

    send(m_socket, (char*)&sizeNet, sizeof(sizeNet), 0);

    int total = 0;
    while (total < jpeg.size())
    {
        int sent = send(
            m_socket,
            (char*)jpeg.data() + total,
            (int)(jpeg.size() - total),
            0
        );

        if (sent <= 0)
        {
            Log("Socket send failed");
            return;
        }

        total += sent;
    }
}

void Reader::SetPipe(HANDLE& hPipe)
{
    m_hPipe = hPipe;
}

void Reader::SetSocket(const SOCKET& socket)
{
    m_socket = socket;
}

void Reader::SignalStop()
{
    SetEvent(m_hStop);
}

Reader::~Reader()
{
    Log("Reader Destructor()");
    if (m_hStop != INVALID_HANDLE_VALUE)
        CloseHandle(m_hStop);
    if (m_hStopped != INVALID_HANDLE_VALUE)
        CloseHandle(m_hStopped);
    Log("Reader Destructor End");
}