#pragma once
#include "Pipe.h"

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
	HANDLE m_hStopWhenClientIsConnected;

	void SignalStop();
	void SignalStopWhenClientIsConnected();
	void SetPipe(NamedPipe* pipe);
	void SetSocket(const SOCKET& socket);

private:
	Writer();
	~Writer();

	BOOL m_bRunning;
	SOCKET m_socket;
	NamedPipe* m_pNamedPipe = nullptr;
	HANDLE m_hClientDisconnected;
	HANDLE m_hSocketEvent;
	bool recvAll(SOCKET s, char* buffer, int totalBytes);
	bool CloseHelper();
};