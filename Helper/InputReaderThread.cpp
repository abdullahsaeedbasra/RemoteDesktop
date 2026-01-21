#include "pch.h"
#include "InputReaderThread.h"
#include "ScreenCaptureThread.h"
#include "Logger.h"

IMPLEMENT_DYNCREATE(InputReaderThread, CWinThread)

InputReaderThread::InputReaderThread() : m_hPipe(INVALID_HANDLE_VALUE)
{
	m_hStopped = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

BOOL InputReaderThread::InitInstance()
{
	return TRUE;
}

int InputReaderThread::Run()
{
	SetEvent(m_hStopped);
	return 0;
}

void InputReaderThread::SetPipe(const HANDLE& pipe)
{
	m_hPipe = pipe;
}

InputReaderThread::~InputReaderThread()
{

}