#pragma once
#include <afxwin.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

class Communicator;

class Listener : public CWinThread
{
	DECLARE_DYNCREATE(Listener)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	HANDLE m_hStop;
	HANDLE m_hStopped;
	HANDLE m_hClientConnectedEvent;
	HANDLE m_hClientDisconnectedEvent;

	void SignalStop();
	SOCKET GetSocket() const;

private:
	Listener();
	~Listener();

	SOCKET m_listeningSocket;
	SOCKET m_socket;
	HANDLE m_hSocketEvent;
};

