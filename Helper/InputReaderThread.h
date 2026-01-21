#pragma once
#include <afxwin.h>

class ScreenCaptureThread;

class InputReaderThread : public CWinThread
{
	DECLARE_DYNCREATE(InputReaderThread)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	void SetPipe(const HANDLE& pipe);

	HANDLE m_hStopped;
	ScreenCaptureThread* m_pScreenThread = nullptr;

private:
	InputReaderThread();
	~InputReaderThread();

	HANDLE m_hPipe;
};

