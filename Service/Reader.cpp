#pragma once
#include "Reader.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(Reader, CWinThread)

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
    OVERLAPPED olForPipe = {};
    olForPipe.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_bRunning = TRUE;
    while (m_bRunning)
    {
        BOOL bRetVal = FALSE;
        DWORD dwFrameSize = 0;
        DWORD dwBytesRead = 0;
        {
            Log("Before framze size read lock");
            CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
            Log("Before framze size read lock");

            bRetVal = ReadFile(m_pNamedPipe->handle, &dwFrameSize, sizeof(dwFrameSize), &dwBytesRead, &olForPipe);
        }

        HANDLE arrHandles[2] = { m_hStop, olForPipe.hEvent };
        int iIndex = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
        if (iIndex == 0)
        {
            Log("Index returned 0");
            m_bRunning = FALSE;
        }
        else if(iIndex == 1)
        {
            bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);

            if (!bRetVal || dwBytesRead != sizeof(dwFrameSize))
            {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_BROKEN_PIPE)
                    Log("Pipe closed by helper");
                else
                    Log("Failed to read pipe header with error" + to_string(dwError));
                m_bRunning = FALSE;
            }
            else
            {
                Log("Frame size received");
                uint32_t uiSize = ntohl(dwFrameSize);

                if (uiSize == 0 || uiSize > 10 * 1024 * 1024)
                {
                    Log("Invalid message size");
                    break;
                }

                std::vector<BYTE> vJpegData(uiSize);
                DWORD dwTotal = 0;
                BOOL bSuccess = TRUE;
                while (dwTotal < uiSize && bSuccess)
                {
                    {
                        Log("Before jpeg read lock");
                        CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
                        Log("Before jpeg read lock");
                        bSuccess = ReadFile(m_pNamedPipe->handle, vJpegData.data() + dwTotal, uiSize - dwTotal, &dwBytesRead, &olForPipe);
                    }

                    HANDLE arrHandles2[2] = { m_hStop, olForPipe.hEvent };
                    int iIndex2 = WaitForMultipleObjects(2, arrHandles2, FALSE, INFINITE) - WAIT_OBJECT_0;

                    if (iIndex2 == 0)
                    {
                        Log("Stop Called for reader");
                        m_bRunning = FALSE;
                    }
                    else
                    {
                        bSuccess = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);
                        if (!bSuccess)
                        {
                            Log("Pipe read failed during jpegData " + to_string(GetLastError()));
                            m_bRunning = FALSE;
                        }
                        else
                        {
                            Log("Jpeg Read");
                            dwTotal += dwBytesRead;
                        }
                    }
                }
                if (bSuccess)
                {
                    Log("Success");
                    SendFrameToSocket(vJpegData);
                }
            }
        }
        else
        {
            Log("Invalid Index");
        }
    }

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
        int sent = send(m_socket, (char*)jpeg.data() + total, (int)(jpeg.size() - total), 0);

        if (sent <= 0)
        {
            Log("Socket send failed");
            return;
        }

        total += sent;
    }
}

void Reader::SetPipe(NamedPipe* Pipe)
{
    m_pNamedPipe = Pipe;
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