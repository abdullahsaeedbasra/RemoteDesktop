#pragma once
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <afxwin.h>

class Writer : public CWinThread
{
	DECLARE_DYNCREATE(Writer)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	HANDLE m_hStop;
	HANDLE m_hStopped;

	void SignalStop();
	void SetPipe(HANDLE& hPipe);
	void SetSocket(const SOCKET& socket);

private:
	Writer();
	~Writer();

	SOCKET m_socket;
	HANDLE m_hPipe;
	HANDLE m_hClientDisconnected;
};

