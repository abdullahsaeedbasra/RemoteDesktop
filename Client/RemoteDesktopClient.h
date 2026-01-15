
// RemoteDesktopClient.h : main header file for the RemoteDesktopClient application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

// CRemoteDesktopClientApp:
// See RemoteDesktopClient.cpp for the implementation of this class
//

class CRemoteDesktopClientApp : public CWinApp
{
public:
	CRemoteDesktopClientApp() noexcept;


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CRemoteDesktopClientApp theApp;
