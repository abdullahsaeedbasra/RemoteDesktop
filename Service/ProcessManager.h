#define _HAS_STD_BYTE 0

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOBYTE
#define _WINSOCKAPI_

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afxwin.h>
#include <windows.h>

class ProcessManager : public CWinThread
{
	DECLARE_DYNCREATE(ProcessManager)

public:
	virtual BOOL InitInstance();
	virtual int Run();
	virtual int ExitInstance();

	void CreateStartEvent();
	void CreateStartFailedEvent();
	void CreateStopEvent();
	void CreateStoppedEvent();
	void SignalStop();

	BOOL StartHelper();

	HANDLE GetStoppedEvent() const;
	HANDLE GetStartEvent() const;
	HANDLE GetStartFailedEvent() const;

private:
	ProcessManager();
	~ProcessManager();

	HANDLE m_hStartEvent;
	HANDLE m_hStartFailedEvent;
	HANDLE m_hStopEvent;
	HANDLE m_hStoppedEvent;
};