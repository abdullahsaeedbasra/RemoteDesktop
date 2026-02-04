
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "ChildView.h"
#include <vector>
using namespace std;

#define WM_NEW_FRAME (WM_USER + 100)
#define WM_NEW_MOUSE_POS (WM_USER + 101)
#define WM_LEFT_BUTTON_DOWN (WM_USER + 102)
#define WM_LEFT_BUTTON_UP (WM_USER + 103)
#define WM_RIGHT_BUTTON_DOWN (WM_USER + 104)
#define WM_RIGHT_BUTTON_UP (WM_USER + 105)
#define WM_LEFT_BUTTON_DOUBLE_CLICK (WM_USER + 106)
#define WM_MOUSE_WHEEL_UP (WM_USER + 107)
#define WM_MOUSE_WHEEL_DOWN (WM_USER + 108)
#define WM_KEY_DOWN (WM_USER + 109)
#define WM_KEY_UP (WM_USER + 110)

class RDCCommunicatorUDP;
class RDCCommunicator;

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame() noexcept;
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;
	CChildView    m_wndView;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg LRESULT OnNewFrame(WPARAM, LPARAM);
	afx_msg LRESULT OnNewMousePos(WPARAM, LPARAM);
	afx_msg LRESULT OnLeftButtonDown(WPARAM, LPARAM);
	afx_msg LRESULT OnLeftButtonUp(WPARAM, LPARAM);
	afx_msg LRESULT OnRightButtonDown(WPARAM, LPARAM);
	afx_msg LRESULT OnRightButtonUp(WPARAM, LPARAM);
	afx_msg LRESULT OnLeftButtonDoubleClick(WPARAM, LPARAM);
	afx_msg LRESULT OnMouseWheelUp(WPARAM, LPARAM);
	afx_msg LRESULT OnMouseWheelDown(WPARAM, LPARAM);
	afx_msg LRESULT OnKeyDown(WPARAM, LPARAM);
	afx_msg LRESULT OnKeyUp(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

private:
	RDCCommunicator* m_pCommunicator = NULL;
	RDCCommunicatorUDP* m_pCommunicatorUDP = NULL;
	
	CSemaphore m_lastJpegLock;
	vector<BYTE> m_lastJpeg;

public:
	void SetLastJpeg(const vector<BYTE>& jpeg);
};


