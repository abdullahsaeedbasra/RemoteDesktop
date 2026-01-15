#pragma once
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <afxwin.h>

class Receiver : public CWinThread
{
	DECLARE_DYNCREATE(Receiver)

public:
	virtual BOOL InitInstance();
	virtual int Run();
	virtual int ExitInstance();

	void SetClientSocket(const SOCKET& socket);
	void SetPipe(const HANDLE& hpipe);
	void CreateStopEvent();
	void CreateStoppedEvent();
	void SignalStop();

	HANDLE GetStoppedEvent() const;

private:
	Receiver();
	~Receiver();

	BOOL m_bRunning;
	SOCKET m_clientSocket;
	HANDLE m_hPipe;
	HANDLE m_hStopEvent;
	HANDLE m_hStoppedEvent;
};

