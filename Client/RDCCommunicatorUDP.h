#pragma once
#include <afxwin.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <map>
#include <gdiplus.h>
using namespace std;

class CMainFrame;

#pragma comment(lib, "Ws2_32.lib")

#pragma pack(push, 1)
struct FramePacketHeader
{
    uint32_t frameId;
    uint16_t packetIndex;
    uint16_t totalPackets;
};
#pragma pack(pop)

struct FrameAssembly
{
    uint16_t totalPackets = 0;
    uint16_t receivedPackets = 0;
    vector<vector<BYTE>> chunks;
};

class RDCCommunicatorUDP : public CWinThread
{
    DECLARE_DYNCREATE(RDCCommunicatorUDP)

public:
    CMainFrame* m_pNotifyWnd = nullptr;
    RDCCommunicatorUDP();

protected:
    virtual BOOL InitInstance();
    virtual int Run();
    virtual int ExitInstance();

private:
    void HandlePacket(char* packet, int size);
    void AssembleFrame(uint32_t frameId);
    void OnFrameReady(const std::vector<BYTE>& jpeg);

    SOCKET m_socket;
    sockaddr_in m_serverService;

    BOOL m_bRumming;

    map<uint32_t, FrameAssembly> m_frames;

    std::vector<BYTE> m_lastJpeg;
};