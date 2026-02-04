#pragma once
#include <afxwin.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "MainFrm.h"
#include "Message.h"

class RDCCommunicator : public CWinThread
{
	DECLARE_DYNCREATE(RDCCommunicator)	

public:
	CMainFrame* m_pNotifyWnd = nullptr;

	virtual BOOL InitInstance();
	virtual int Run();
	virtual int ExitInstance();

	void SendMouseMove(int, int);
	void SendLeftButtonDown();
	void SendLeftButtonUp();
	void SendRightButtonDown();
	void SendRightButtonUp();
	void SendLeftButtonDoubleClick();
	void SendMouseWheelUp();
	void SendMouseWheelDown();
	void SendKeyDown(KeyData* pData);
	void SendKeyUp(KeyData* pData);
private:
	RDCCommunicator();
	~RDCCommunicator() { m_bRunning = FALSE; };

	BOOL m_bRunning;
	SOCKET m_socket;

	bool sendAll(SOCKET socket, char* buffer, int totalBytes);
	bool recvAll(SOCKET s, char* buffer, int totalBytes);
};

