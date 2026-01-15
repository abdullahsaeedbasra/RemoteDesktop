#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afxwin.h>

class Sender : public CWinThread
{
	DECLARE_DYNCREATE(Sender)

public:
	virtual BOOL InitInstance();
	virtual int Run();
	virtual int ExitInstance();

	void SetPipe(const HANDLE& hPipe);
	void CreateClientEvent();
	void CreateStopEvent();
	void CreateStoppedEvent();
	void SignalStop();

	HANDLE GetClientEvent() const;
	HANDLE GetStoppedEvent() const;
	SOCKET GetCLientSocket() const;

private:
	Sender();
	~Sender();

	void SendScreen();

	BOOL m_bRunning;
	SOCKET m_hServerSocket;
	SOCKET m_clientSocket;
	HANDLE m_hPipe;
	HANDLE m_hClientEvent;
	HANDLE m_hStopEvent;
	HANDLE m_hStoppedEvent;
};