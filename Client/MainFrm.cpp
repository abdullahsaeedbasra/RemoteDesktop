
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "pch.h"
#include "framework.h"
#include "RemoteDesktopClient.h"

#include "MainFrm.h"
#include "RDCCommunicator.h"
#include "RDCCommunicatorUDP.h"
#include "Message.h"
#include "RDCLogger.h"

#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_MESSAGE(WM_NEW_FRAME, OnNewFrame)
	ON_MESSAGE(WM_NEW_MOUSE_POS, OnNewMousePos)
	ON_MESSAGE(WM_LEFT_BUTTON_DOWN, OnLeftButtonDown)
	ON_MESSAGE(WM_LEFT_BUTTON_UP, OnLeftButtonUp)
	ON_MESSAGE(WM_RIGHT_BUTTON_DOWN, OnRightButtonDown)
	ON_MESSAGE(WM_RIGHT_BUTTON_UP, OnRightButtonUp)
	ON_MESSAGE(WM_LEFT_BUTTON_DOUBLE_CLICK, OnLeftButtonDoubleClick)
	ON_MESSAGE(WM_MOUSE_WHEEL_UP, OnMouseWheelUp)
	ON_MESSAGE(WM_MOUSE_WHEEL_DOWN, OnMouseWheelDown)
	ON_MESSAGE(WM_KEY_DOWN, OnKeyDown)
	ON_MESSAGE(WM_KEY_UP, OnKeyUp)


	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame() noexcept
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, nullptr))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}
	m_wndView.m_pMainFrame = this;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CFrameWnd::OnShowWindow(bShow, nStatus);

	if (!m_pCommunicator)
	{
		m_pCommunicator = (RDCCommunicator*)AfxBeginThread(RUNTIME_CLASS(RDCCommunicator), THREAD_PRIORITY_NORMAL, 0,
															CREATE_SUSPENDED);
		m_pCommunicator->m_pNotifyWnd = this;
		m_pCommunicator->ResumeThread();
	}
}

void CMainFrame::SetLastJpeg(const vector<BYTE>& jpeg)
{
	m_lastJpegLock.Lock();
	m_lastJpeg = jpeg;
	m_lastJpegLock.Unlock();
}

LRESULT CMainFrame::OnNewFrame(WPARAM wparam, LPARAM lparam)
{
	m_lastJpegLock.Lock();
	m_wndView.SetLastJpeg(m_lastJpeg);
	m_lastJpegLock.Unlock();

	m_wndView.PostMessage(WM_NEW_FRAME, NULL, NULL);

	return 0;
}

LRESULT CMainFrame::OnNewMousePos(WPARAM wparam, LPARAM lparam)
{
	int percentage_x = (int)(wparam);
	int percentage_y = (int)(lparam);
	m_pCommunicator->SendMouseMove(percentage_x, percentage_y);

	return 0;
}

LRESULT CMainFrame::OnLeftButtonDown(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendLeftButtonDown();
	return 0;
}

LRESULT CMainFrame::OnLeftButtonUp(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendLeftButtonUp();
	return 0;
}

LRESULT CMainFrame::OnRightButtonDown(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendRightButtonDown();
	return 0;
}

LRESULT CMainFrame::OnRightButtonUp(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendRightButtonUp();
	return 0;
}

LRESULT CMainFrame::OnLeftButtonDoubleClick(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendLeftButtonDoubleClick();
	return 0;
}

LRESULT CMainFrame::OnMouseWheelUp(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendMouseWheelUp();
	return 0;
}

LRESULT CMainFrame::OnMouseWheelDown(WPARAM wparam, LPARAM lparam)
{
	m_pCommunicator->SendMouseWheelDown();
	return 0;
}

LRESULT CMainFrame::OnKeyDown(WPARAM wparam, LPARAM lparam)
{
	KeyData* pData = (KeyData*)lparam;
	m_pCommunicator->SendKeyDown(pData);
	return 0;
}

LRESULT CMainFrame::OnKeyUp(WPARAM wparam, LPARAM lparam)
{
	KeyData* pData = (KeyData*)lparam;
	m_pCommunicator->SendKeyUp(pData);
	return 0;
}
