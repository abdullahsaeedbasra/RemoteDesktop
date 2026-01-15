#pragma once
#include <afxwin.h>
class ScreenCaptureThread : public CWinThread
{
	DECLARE_DYNCREATE(ScreenCaptureThread)

public:
	virtual BOOL InitInstance();
	virtual int Run();
	virtual int ExitInstance();

	void SetPipe(const HANDLE& hPipe);

private:
	ScreenCaptureThread();
	~ScreenCaptureThread();

	void SendScreenFrame();
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	BOOL m_bRunning;
	HANDLE m_hPipe;

	ULONG_PTR m_gdiplusToken;
	uint32_t m_frameId;

	ULONGLONG m_lastFrameTime;
	UINT m_frameRate;
};