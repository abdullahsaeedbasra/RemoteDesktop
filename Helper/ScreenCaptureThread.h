#pragma once
#include "Pipe.h"
#include <afxwin.h>
#include <vector>

class ScreenCaptureThread : public CWinThread
{
	DECLARE_DYNCREATE(ScreenCaptureThread)

public:
	virtual BOOL InitInstance();
	virtual int Run();

	void SetPipe(NamedPipe* pipe);
	void SignalStop();
	HANDLE m_hStop;
	HANDLE m_hStopped;

private:
	ScreenCaptureThread();
	~ScreenCaptureThread();

	std::vector<BYTE> CaptureScreenAsJpeg();
	void SendScreenFrame();
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	NamedPipe* m_pNamedPipe = nullptr;
	ULONG_PTR m_gdiplusToken;
	ULONGLONG m_lastFrameTime;
	UINT m_frameRate;
};