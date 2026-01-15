
// ChildView.h : interface of the CChildView class
//


#pragma once
#include <atlimage.h>
#include <afxmt.h>
#include <vector>

#define WM_NEW_FRAME (WM_USER + 100)

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
};

