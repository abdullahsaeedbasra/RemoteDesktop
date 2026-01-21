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

IMPLEMENT_DYNCREATE(ScreenCaptureThread, CWinThread)

ScreenCaptureThread::ScreenCaptureThread() : m_hPipe(INVALID_HANDLE_VALUE), m_gdiplusToken(0),
                                             m_lastFrameTime(0), m_frameRate(0)
{ 
    m_hStop = CreateEvent(nullptr, FALSE, FALSE, L"StopScreenCapture");
    m_hStopped = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

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
    m_lastFrameTime = GetTickCount64();
    m_frameRate = 20;

    while (::WaitForSingleObject(m_hStop, 0) != WAIT_OBJECT_0)
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
    SetEvent(m_hStopped);
    return 0;
}

void ScreenCaptureThread::SignalStop()
{
    SetEvent(m_hStop);
}

void ScreenCaptureThread::SetPipe(const HANDLE& hPipe)
{
    m_hPipe = hPipe;
}

void ScreenCaptureThread::SendScreenFrame()
{
    std::vector<BYTE> jpegData = CaptureScreenAsJpeg();
    uint32_t jpegSize = (uint32_t)jpegData.size();

    PipeMessageHeader header{};
    header.type = MSG_SCREEN;
    header.size = htonl(jpegSize);

    DWORD written = 0;

    if (!WriteFile(m_hPipe, &header, sizeof(header), &written, nullptr) ||
        written != sizeof(header))
    {
        Log("Failed to write screen header");
        return;
    }

    DWORD total = 0;
    while (total < jpegSize)
    {
        DWORD chunk = std::min<DWORD>(65536u, jpegSize - total);
        DWORD w = 0;

        if (!WriteFile(m_hPipe,
            jpegData.data() + total,
            chunk,
            &w,
            nullptr))
        {
            Log("Pipe write failed during screen data");
            return;
        }
        total += w;
    }
}

std::vector<BYTE> ScreenCaptureThread::CaptureScreenAsJpeg()
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
        std::vector<BYTE> vec(1, 1);
        Log("JPEG encoder not found");
        stream->Release();
        SelectObject(hMemDC, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hScreenDC);
        return vec;
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

    return jpegData;
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

ScreenCaptureThread::~ScreenCaptureThread()
{
}