
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "ChildView.h"
#include <vector>
using namespace std;

#define WM_NEW_FRAME (WM_USER + 100)

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
	DECLARE_MESSAGE_MAP()

private:
	RDCCommunicator* m_pCommunicator = NULL;
	RDCCommunicatorUDP* m_pCommunicatorUDP = NULL;
	
	CSemaphore m_lastJpegLock;
	vector<BYTE> m_lastJpeg;

public:
	void SetLastJpeg(const vector<BYTE>& jpeg);
};


