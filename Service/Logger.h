#pragma once
#include <afxwin.h>
#include <afxmt.h>
#include <fstream>
#include <string>
#include <queue>
using namespace std;

#define Log(s) HelperLogger::Instance().writeLog(s)

class HelperLogger : public CWinThread
{
    DECLARE_DYNCREATE(HelperLogger)
public:
    static HelperLogger& Instance()
    {
        static HelperLogger* pInstance = NULL;
        if (!pInstance)
        {
            pInstance = (HelperLogger*)AfxBeginThread(RUNTIME_CLASS(HelperLogger),
                THREAD_PRIORITY_NORMAL,
                0,
                CREATE_SUSPENDED);

            if (pInstance)
            {
                pInstance->m_bAutoDelete = FALSE;
                pInstance->ResumeThread();
            }
        }
        return *pInstance;
    }
    void writeLog(const string& logMessage);

    virtual int Run();

private:
    HelperLogger();
    ~HelperLogger();

    bool m_bRunning;
    CSemaphore m_logQueueLock;
    queue<CString> m_logQueue;
    CEvent m_logEvent;
    CString m_buffer;

    CString GetCurrentTimeString();
    CString GenerateLogFileName();
    void FlushBufferToFile();

public:
    virtual BOOL InitInstance();
};
