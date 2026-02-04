#define _HAS_STD_BYTE 0

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOBYTE
#define _WINSOCKAPI_

#include "Pipe.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afxwin.h>
#include <windows.h>

class Reader;
class Writer;

class ProcessManager : public CWinThread
{
	DECLARE_DYNCREATE(ProcessManager)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	HANDLE m_hStop;
	HANDLE m_hStopped;
	HANDLE m_hStopWhenClientIsConnected;

	void SignalStop();
	void SetSocket(const SOCKET& socket);
	void SignalStopWhenClientIsConnected();

private:
	ProcessManager();
	~ProcessManager();

	NamedPipe* m_pNamedPipe = nullptr;
	Reader* m_pReader = nullptr;
	Writer* m_pWriter = nullptr;
	SOCKET m_socket;

	void CreatePipe();
	BOOL StartHelper();
	BOOL WaitForHelper();
};