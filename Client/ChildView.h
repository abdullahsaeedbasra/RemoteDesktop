
// ChildView.h : interface of the CChildView class
//


#pragma once
#include <atlimage.h>
#include <afxmt.h>
#include <vector>

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

class CMainFrame;
// CChildView window

class CChildView : public CWnd
{
// Construction
public:
	CChildView();

// Attributes
public:

// Operations
public:

// Overrides
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
public:
	virtual ~CChildView();
	void SetLastJpeg(const std::vector<BYTE>& jpeg);

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

private:
	CSemaphore m_frameLock;
	CImage m_frame;

	LRESULT OnNewFrame(WPARAM, LPARAM);

	CSemaphore m_lastJpegLock;
	std::vector<BYTE> m_latestJPEG;
public:
	CMainFrame* m_pMainFrame = nullptr;
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
};

