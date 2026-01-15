#pragma once
#include <afxwin.h>
#include <afxmt.h>
#include <fstream>
#include <string>
#include <queue>
using namespace std;

#define Log(s) RDCLogger::Instance().writeLog(s)

class RDCLogger : public CWinThread
{
    DECLARE_DYNCREATE(RDCLogger)
public:
    static RDCLogger& Instance()
    {
        static RDCLogger* pInstance = NULL;
        if (!pInstance)
        {
            pInstance = (RDCLogger*)AfxBeginThread(RUNTIME_CLASS(RDCLogger),
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
    RDCLogger();
    ~RDCLogger();

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

