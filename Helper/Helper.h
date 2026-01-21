
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "ScreenCaptureThread.h"
#include "InputReaderThread.h"

class CHelperApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	HANDLE ConnectToServicePipe();

private:
	ScreenCaptureThread* m_pScreenThread = nullptr;
	InputReaderThread* m_pInputReader = nullptr;
public:
	virtual int Run();
};
