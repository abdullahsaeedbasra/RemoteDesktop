
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "RemoteDesktopClient.h"
#include "ChildView.h"
#include "RDCLogger.h"
#include "RDCCommunicatorUDP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_MESSAGE(WM_NEW_FRAME, OnNewFrame)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnPaint()
{
    Log("Inside OnPaint");
    CPaintDC dc(this);

    CRect rect;
    GetClientRect(&rect);

    m_frameLock.Lock();

    if (!m_frame.IsNull())
    {
        // High-quality stretch
        SetStretchBltMode(dc.m_hDC, HALFTONE);

        m_frame.Draw(
            dc.m_hDC,
            rect.left,
            rect.top,
            rect.Width(),
            rect.Height()
        );
    }
    else
    {
        dc.FillSolidRect(rect, RGB(0, 0, 0));
    }
    m_frameLock.Unlock();
}

LRESULT CChildView::OnNewFrame(WPARAM, LPARAM)
{
    Log("On New Frame");
    IStream* stream = nullptr;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    stream->Write(m_latestJPEG.data(), (ULONG)m_latestJPEG.size(), nullptr);

    LARGE_INTEGER pos{};
    stream->Seek(pos, STREAM_SEEK_SET, nullptr);

    m_frame.Destroy();
    m_frame.Load(stream);

    stream->Release();

    Invalidate(FALSE);
    return 0;
}

void CChildView::SetLastJpeg(const std::vector<BYTE>& jpeg)
{
    Log("Set Last jPeg");
    m_lastJpegLock.Lock();
    m_latestJPEG = jpeg;
    m_lastJpegLock.Unlock();
    Log("1");
}