#pragma once
#include <afxwin.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "MainFrm.h"

class RDCCommunicator : public CWinThread
{
	DECLARE_DYNCREATE(RDCCommunicator)	

public:
	CMainFrame* m_pNotifyWnd = nullptr;

	virtual BOOL InitInstance();
	virtual int Run();
	virtual int ExitInstance();

private:
	RDCCommunicator();
	~RDCCommunicator() { m_bRunning = FALSE; };

	BOOL m_bRunning;
	SOCKET m_socket;
};

