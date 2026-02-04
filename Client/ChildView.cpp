
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "RemoteDesktopClient.h"
#include "ChildView.h"
#include "RDCLogger.h"
#include "RDCCommunicatorUDP.h"
#include "MainFrm.h"
#include "Message.h"

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
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_MOUSEWHEEL()
    ON_WM_KEYDOWN()
    ON_WM_KEYUP()
    ON_WM_ACTIVATE()
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
        SetStretchBltMode(dc.m_hDC, HALFTONE);

        m_frame.Draw(dc.m_hDC, rect.left, rect.top, rect.Width(), rect.Height());
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

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    CRect clientRect;
    GetClientRect(&clientRect);

    int viewWidth = clientRect.Width();
    int viewHeight = clientRect.Height();

    int percentage_x = (point.x * 100) / viewWidth;
    int percentage_y = (point.y * 100) / viewHeight;

    m_pMainFrame->PostMessage(WM_NEW_MOUSE_POS, (WPARAM)percentage_x, (LPARAM)percentage_y);
    CWnd::OnMouseMove(nFlags, point);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_pMainFrame->PostMessage(WM_LEFT_BUTTON_DOWN, 0, 0);
    CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_pMainFrame->PostMessage(WM_LEFT_BUTTON_UP, 0, 0);
    CWnd::OnLButtonUp(nFlags, point);
}

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    m_pMainFrame->PostMessage(WM_RIGHT_BUTTON_DOWN, 0, 0);
    CWnd::OnRButtonDown(nFlags, point);
}

void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
    m_pMainFrame->PostMessage(WM_RIGHT_BUTTON_UP, 0, 0);
    CWnd::OnRButtonUp(nFlags, point);
}

void CChildView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    m_pMainFrame->PostMessageW(WM_LEFT_BUTTON_DOUBLE_CLICK, 0, 0);

    CWnd::OnLButtonDblClk(nFlags, point);
}

BOOL CChildView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (zDelta > 0)
        m_pMainFrame->PostMessage(WM_MOUSE_WHEEL_UP, 0, 0);
    else if (zDelta < 0)
        m_pMainFrame->PostMessageW(WM_MOUSE_WHEEL_DOWN, 0, 0);

    return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    Log("OnKeyDown");
    KeyData* pKeyData = new KeyData{};
    pKeyData->flags = 0;
    if (nFlags & KF_EXTENDED)
    {
        pKeyData->flags |= KEYEVENTF_EXTENDEDKEY;
    }
    pKeyData->vk = (uint32_t)nChar;

    m_pMainFrame->PostMessageW(WM_KEY_DOWN, 0, (LPARAM)pKeyData);

    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CChildView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    Log("OnKeyUp");
    KeyData* pKeyData = new KeyData{};
    pKeyData->flags = KEYEVENTF_KEYUP;
    if (nFlags & KF_EXTENDED)
    {
        pKeyData->flags |= KEYEVENTF_EXTENDEDKEY;
    }
    pKeyData->vk = (uint32_t)nChar;

    m_pMainFrame->PostMessageW(WM_KEY_UP, 0, (LPARAM)pKeyData);

    CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}
BOOL CChildView::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
    {
        OnKeyDown((UINT)pMsg->wParam, 1, (UINT)pMsg->lParam);
        return TRUE;
    }

    if (pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP)
    {
        OnKeyUp((UINT)pMsg->wParam, 1, (UINT)pMsg->lParam);
        return TRUE;
    }

    return CWnd::PreTranslateMessage(pMsg);
}