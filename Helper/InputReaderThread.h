#pragma once
#include "Pipe.h"

#include <afxwin.h>

class ScreenCaptureThread;

class InputReaderThread : public CWinThread
{
	DECLARE_DYNCREATE(InputReaderThread)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	void SetPipe(NamedPipe* pipe);

	HANDLE m_hStopped;
	ScreenCaptureThread* m_pScreenThread = nullptr;

private:
	InputReaderThread();
	~InputReaderThread();

	BOOL m_bRunning;
	NamedPipe* m_pNamedPipe = nullptr;
};

