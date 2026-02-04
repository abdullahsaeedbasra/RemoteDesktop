#pragma once
#include "Pipe.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afxwin.h>
#include <vector>

class Reader : public CWinThread
{
	DECLARE_DYNCREATE(Reader)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	HANDLE m_hStop;
	HANDLE m_hStopped;

	void SignalStop();
	void SetPipe(NamedPipe* Pipe);
	void SetSocket(const SOCKET& socket);

private:
	Reader();
	~Reader();

	SOCKET m_socket;
	BOOL m_bRunning;
	NamedPipe* m_pNamedPipe = nullptr;

	void SendFrameToSocket(const std::vector<BYTE>& jpeg, uint32_t frameSize);
	bool SendAll(SOCKET socket, char* buffer, int totalBytes);
};