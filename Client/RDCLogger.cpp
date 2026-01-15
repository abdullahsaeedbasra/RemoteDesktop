#include "pch.h"
#include "RDCLogger.h"

IMPLEMENT_DYNCREATE(RDCLogger, CWinThread)

RDCLogger::RDCLogger() : m_bRunning(false), m_logEvent(FALSE, TRUE),
m_buffer(L"")
{
	m_bAutoDelete = FALSE;
}

BOOL RDCLogger::InitInstance()
{
	return TRUE;
}

int RDCLogger::Run()
{
	m_bRunning = true;
	while (m_bRunning)
	{
		::WaitForSingleObject(m_logEvent.m_hObject, INFINITE);

		while (true)
		{
			m_logQueueLock.Lock();
			bool isEmpty = m_logQueue.empty();
			m_logQueueLock.Unlock();

			if (isEmpty)
			{
				FlushBufferToFile();
				m_buffer = L"";
				m_logEvent.ResetEvent();
				break;
			}

			m_logQueueLock.Lock();
			CString logMessage = m_logQueue.front();
			m_logQueue.pop();
			m_logQueueLock.Unlock();

			m_buffer += logMessage;
			if (m_buffer.GetLength() > 20480)
			{
				FlushBufferToFile();
				m_buffer = L"";
			}
		}
		Sleep(10);
	}
	return 0;
}

CString RDCLogger::GetCurrentTimeString()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	CString timeStr;
	timeStr.Format(_T("%02d:%02d:%02d.%03d"),
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);

	return timeStr;
}

CString RDCLogger::GenerateLogFileName()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	CString fileName;
	fileName.Format(_T("RDS_%04d-%02d-%02d_%02d.log"),
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour);

	return fileName;
}

void RDCLogger::writeLog(const string& logMessage)
{
	CString timeStr = GetCurrentTimeString();
	CString msg(logMessage.c_str());

	CString line;
	line.Format(_T("%s %s\n"),
		timeStr.GetString(),
		msg.GetString());

	m_logQueueLock.Lock();
	m_logQueue.push(line);
	m_logQueueLock.Unlock();
	m_logEvent.SetEvent();
}

void RDCLogger::FlushBufferToFile()
{
	if (m_buffer.IsEmpty())
		return;

	ofstream file;
	CString fileName = GenerateLogFileName();

	file.open(CT2A(fileName), std::ios::out | std::ios::app);
	if (!file.is_open())
		return;

	file << CT2A(m_buffer);
	file.flush();

	file.close();
}

RDCLogger::~RDCLogger()
{
	m_bRunning = false;
}