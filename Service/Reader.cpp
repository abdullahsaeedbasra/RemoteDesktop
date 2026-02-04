#pragma once
#include "Reader.h"
#include "Logger.h"
#include "Message.h"

IMPLEMENT_DYNCREATE(Reader, CWinThread)

Reader::Reader() : m_socket(INVALID_SOCKET), m_bRunning(FALSE)
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
        DWORD frameSize;
        BOOL bRetVal = FALSE;
        DWORD dwBytesRead = 0;
        {
            CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
            ReadFile(m_pNamedPipe->handle, &frameSize, sizeof(frameSize), &dwBytesRead, &olForPipe);
        }

        HANDLE arrHandles[2] = { m_hStop, olForPipe.hEvent };
        int iIndex = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
        if (iIndex == 0)
        {
            Log("Reader: Stop Signaled");
            m_bRunning = FALSE;
        }
        else if(iIndex == 1)
        {
            bRetVal = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);

            if (!bRetVal)
            {
                m_bRunning = FALSE;
            }
            else
            {
                uint32_t uiSize = ntohl(frameSize);

                if (uiSize == 0 || uiSize > 10 * 1024 * 1024)
                {
                    m_bRunning = FALSE;
                }
                else
                {
                    std::vector<BYTE> vJpegData(uiSize);
                    DWORD dwTotal = 0;
                    BOOL bSuccess = TRUE;
                    while (dwTotal < uiSize && bSuccess)
                    {
                        {
                            CSingleLock lock(&m_pNamedPipe->csSynchronizer, TRUE);
                            ReadFile(m_pNamedPipe->handle, vJpegData.data() + dwTotal, uiSize - dwTotal, &dwBytesRead, &olForPipe);
                        }

                        HANDLE arrHandles2[2] = { m_hStop, olForPipe.hEvent };
                        int iIndex2 = WaitForMultipleObjects(2, arrHandles2, FALSE, INFINITE) - WAIT_OBJECT_0;

                        if (iIndex2 == 0)
                        {
                            Log("Reader: Stop Signaled");
                            m_bRunning = FALSE;
                        }
                        else
                        {
                            bSuccess = GetOverlappedResult(m_pNamedPipe->handle, &olForPipe, &dwBytesRead, FALSE);
                            if (!bSuccess)
                            {
                                Log("Pipe read failed during jpegData " + to_string(GetLastError()));
                                continue;
                            }
                            else
                            {
                                dwTotal += dwBytesRead;
                            }
                        }
                    }
                    if (bSuccess)
                    {
                        Log("Reader: Frame read from the pipe");;
                        SendFrameToSocket(vJpegData, uiSize);
                    }
                }
            }
        }
    }
    Log("Reader Exiting");
    SetEvent(m_hStopped);
    return 0;
}

void Reader::SendFrameToSocket(const std::vector<BYTE>& jpeg, uint32_t frameSize)
{
    if (frameSize > 10 * 1024 * 1024)
        return;
        
    frameSize = htonl(frameSize);

    if (!SendAll(m_socket, (char*)&frameSize, sizeof(frameSize)))
        return;

    if (!SendAll(m_socket, (char*)jpeg.data(), jpeg.size()))
        return;

    Log("Reader: Frame Forwarded");
}

bool Reader::SendAll(SOCKET socket, char* buffer, int totalBytes)
{
    int bytesSent = 0;
    while (bytesSent < totalBytes)
    {
        int sent = send(socket, buffer + bytesSent, totalBytes - bytesSent, 0);
        if (sent == SOCKET_ERROR)
            return false;

        bytesSent = sent;
    }
    return true;
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
    m_bRunning = FALSE;
    if (m_hStop != INVALID_HANDLE_VALUE)
        CloseHandle(m_hStop);
    if (m_hStopped != INVALID_HANDLE_VALUE)
        CloseHandle(m_hStopped);
    Log("Reader Destructor End");
}