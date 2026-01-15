#define _HAS_STD_BYTE 0
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "pch.h"

#include "ScreenCaptureThread.h"
#include "Logger.h"

#include <vector>
#include <string>
#include <wchar.h>
#include <algorithm>
#include <gdiplus.h>
#include <ws2tcpip.h>

using namespace std;
using namespace Gdiplus;

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")

#pragma pack(push, 1)
struct FramePacketHeader
{
    uint32_t frameId;
    uint16_t packetIndex;
    uint16_t totalPackets;
};
#pragma pack(pop)

IMPLEMENT_DYNCREATE(ScreenCaptureThread, CWinThread)

ScreenCaptureThread::ScreenCaptureThread() : m_bRunning(FALSE), m_hPipe(INVALID_HANDLE_VALUE), m_gdiplusToken(0),
                                             m_frameId(0), m_lastFrameTime(0), m_frameRate(0)
{ }

BOOL ScreenCaptureThread::InitInstance()
{
    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL) != Ok)
    {
        Log("GDI+ startup failed");
        return FALSE;
    }
	return TRUE;
}

int ScreenCaptureThread::Run()
{
    Log("ScreenCaptureThread Run Started");
    m_bRunning = TRUE;
    m_lastFrameTime = GetTickCount64();
    m_frameRate = 15;

    while (m_bRunning)
    {
        DWORD currentTime = GetTickCount64();
        DWORD frameInterval = 1000 / m_frameRate;

        if (currentTime - m_lastFrameTime >= frameInterval)
        {
            SendScreenFrame();
            m_lastFrameTime = currentTime;
        }
        else
        {
            DWORD sleepTime = frameInterval - (currentTime - m_lastFrameTime);
            if (sleepTime > 0)
            {
                if (sleepTime > 10)
                    Sleep(10);
                else
                    Sleep(sleepTime);
            }
        }
    }

    Log("ScreenCaptureThread Run Ended");
    return 0;
}

void ScreenCaptureThread::SendScreenFrame()
{
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    HGDIOBJ oldObj = SelectObject(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, w, h, hScreenDC, 0, 0, SRCCOPY | CAPTUREBLT);

    IStream* stream = nullptr;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);

    Bitmap bmp(hBitmap, NULL);

    CLSID jpegClsid;
    if (GetEncoderClsid(L"image/jpeg", &jpegClsid) < 0)
    {
        Log("JPEG encoder not found");
        stream->Release();
        SelectObject(hMemDC, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hScreenDC);
        return;
    }

    EncoderParameters params{};
    params.Count = 1;
    params.Parameter[0].Guid = EncoderQuality;
    params.Parameter[0].Type = EncoderParameterValueTypeLong;
    params.Parameter[0].NumberOfValues = 1;

    ULONG quality = 50;
    params.Parameter[0].Value = &quality;

    bmp.Save(stream, &jpegClsid, &params);

    STATSTG stat{};
    stream->Stat(&stat, STATFLAG_NONAME);

    ULONG jpegSize = stat.cbSize.LowPart;
    vector<BYTE> jpegData(jpegSize);

    LARGE_INTEGER pos{};
    stream->Seek(pos, STREAM_SEEK_SET, NULL);

    ULONG read = 0;
    stream->Read(jpegData.data(), jpegSize, &read);

    stream->Release();
    SelectObject(hMemDC, oldObj);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    uint32_t frameSizeNetwork = htonl(jpegSize);

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(
        m_hPipe,
        &frameSizeNetwork,
        sizeof(frameSizeNetwork),
        &bytesWritten,
        NULL
    );

    if (!ok || bytesWritten != sizeof(frameSizeNetwork))
    {
        Log("Failed to write frame size to pipe. Error: " + std::to_string(GetLastError()));
        return;
    }

    DWORD totalWritten = 0;

    while (totalWritten < jpegSize)
    {
        DWORD remaining = jpegSize - totalWritten;
        DWORD chunkSize = min(remaining, 65536UL);

        DWORD bytesWritten = 0;
        BOOL ok = WriteFile(
            m_hPipe,
            jpegData.data() + totalWritten,
            chunkSize,
            &bytesWritten,
            NULL
        );

        if (!ok)
        {
            DWORD error = GetLastError();

            if (error == ERROR_NO_DATA || error == ERROR_BROKEN_PIPE)
            {
                Log("Pipe disconnected");
                return;
            }

            Log("Error writing frame data to pipe: " + std::to_string(error));
            return;
        }

        totalWritten += bytesWritten;
    }

    string logMsg = "Frame " + to_string(m_frameId) +
        " sent (" + to_string(jpegSize) + " bytes)";
    Log(logMsg);

    m_frameId++;
}

int ScreenCaptureThread::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;
    UINT size = 0;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;

    std::vector<BYTE> buffer(size);
    Gdiplus::ImageCodecInfo* pImageCodecInfo =
        reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT i = 0; i < num; ++i)
    {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[i].Clsid;
            return i;
        }
    }
    return -1;
}

void ScreenCaptureThread::SetPipe(const HANDLE& hPipe)
{
	m_hPipe = hPipe;
}

int ScreenCaptureThread::ExitInstance()
{
	return CWinThread::ExitInstance();
}

ScreenCaptureThread::~ScreenCaptureThread()
{
	m_bRunning = FALSE;
}