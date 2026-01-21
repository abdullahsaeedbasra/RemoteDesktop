#define _HAS_STD_BYTE 0

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOBYTE
#define _WINSOCKAPI_

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afxwin.h>
#include <afxmt.h>
#include <windows.h>

#define SERVICE_NAME _T("RemoteDesktopServerService")

class RemoteDesktopService
{
public:
    static RemoteDesktopService& Instance();
    VOID Run();

private:
    RemoteDesktopService() = default;
    ~RemoteDesktopService() = default;

    static VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    static VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode);

    SERVICE_STATUS        m_Status{};
    SERVICE_STATUS_HANDLE m_StatusHandle{};
    HANDLE                m_StopEvent{};

    static RemoteDesktopService* s_Instance;
};

