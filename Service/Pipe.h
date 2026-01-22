#pragma once
#include <afxmt.h>

struct NamedPipe
{
	CCriticalSection csSynchronizer;
	HANDLE handle;
};