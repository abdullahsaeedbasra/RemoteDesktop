#include "pch.h"
#include "RDCCommunicatorUDP.h"
#include "RDCLogger.h"
#include "ChildView.h"
#include "MainFrm.h"

#include <vector>
#include <map>
#include <string>
#include <gdiplus.h>
#include <ws2tcpip.h>

using namespace std;

#define WM_NEW_FRAME (WM_USER + 100)

IMPLEMENT_DYNCREATE(RDCCommunicatorUDP, CWinThread)

RDCCommunicatorUDP::RDCCommunicatorUDP()
    : m_bRumming(FALSE),
    m_socket(INVALID_SOCKET)
{
}

BOOL RDCCommunicatorUDP::InitInstance()
{
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET)
    {
        Log("UDP client socket creation failed");
        return FALSE;
    }

    m_serverService.sin_family = AF_INET;
    m_serverService.sin_port = htons(7000);
    InetPton(AF_INET, L"127.0.0.1", &m_serverService.sin_addr);

    const char* msg = "Hello From UDP Client";
    sendto(m_socket, msg, (int)strlen(msg), 0,
        (sockaddr*)&m_serverService, sizeof(m_serverService));

    Log("UDP discovery message sent");
    return TRUE;
}

int RDCCommunicatorUDP::Run()
{
    Log("UDP Client Run()");
    m_bRumming = TRUE;

    char packet[1500];

    while (m_bRumming)
    {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(m_socket, &readset);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        if (select(0, &readset, NULL, NULL, &timeout) > 0)
        {
            sockaddr_in from{};
            int fromLen = sizeof(from);

            int bytes = recvfrom(
                m_socket,
                packet,
                sizeof(packet),
                0,
                (sockaddr*)&from,
                &fromLen
            );

            if (bytes > sizeof(FramePacketHeader))
            {
                HandlePacket(packet, bytes);
            }
        }
        Sleep(10);
    }
    return 0;
}

void RDCCommunicatorUDP::HandlePacket(char* packet, int size)
{
    FramePacketHeader hdr{};
    memcpy(&hdr, packet, sizeof(hdr));

    string str = "Received packet: frame=" + to_string(hdr.frameId) + ", index=" + to_string(hdr.packetIndex)
        + "/" + to_string(hdr.totalPackets) + ", size=" + to_string(size);
    Log(str);
    str = "";

    // Validate the packet index
    if (hdr.packetIndex >= hdr.totalPackets)
    {
        Log("Invalid packet index received: %d, total: %d");
        return;
    }

    BYTE* payload = (BYTE*)packet + sizeof(hdr);
    int payloadSize = size - sizeof(hdr);

    auto& frame = m_frames[hdr.frameId];

    // Resize if empty
    if (frame.chunks.empty())
    {
        frame.totalPackets = hdr.totalPackets;
        frame.chunks.resize(hdr.totalPackets);
    }
    else if (frame.totalPackets != hdr.totalPackets)
    {
        // If total packets changed, reinitialize

        Log("Total packets changed for frame %d: old=%d, new=%d");
        frame.totalPackets = hdr.totalPackets;
        frame.chunks.clear();
        frame.chunks.resize(hdr.totalPackets);
        frame.receivedPackets = 0;
    }

    // Check bounds again (just to be safe)
    if (hdr.packetIndex >= frame.chunks.size())
    {
        Log("Packet index out of bounds: %d >= %d");
        return;
    }

    // Only process if not already received
    if (frame.chunks[hdr.packetIndex].empty())
    {
        frame.chunks[hdr.packetIndex].assign(payload, payload + payloadSize);
        frame.receivedPackets++;
    }

    if (frame.receivedPackets == frame.totalPackets)
    {
        Log("All Packets received");
        AssembleFrame(hdr.frameId);
        m_frames.erase(hdr.frameId);
    }
}

void RDCCommunicatorUDP::AssembleFrame(uint32_t frameId)
{
    Log("Assemble Frame");
    auto it = m_frames.find(frameId);
    if (it == m_frames.end())
        return;

    // Check if all chunks are non-empty
    for (const auto& chunk : it->second.chunks)
    {
        if (chunk.empty())
        {
            Log("Frame %d has empty chunks, cannot assemble");
            return;
        }
    }

    vector<BYTE> jpegData;
    size_t totalSize = 0;

    // Calculate total size first
    for (auto& chunk : it->second.chunks)
        totalSize += chunk.size();

    jpegData.reserve(totalSize);

    // Assemble data
    for (auto& chunk : it->second.chunks)
        jpegData.insert(jpegData.end(), chunk.begin(), chunk.end());

    m_lastJpeg = std::move(jpegData);

    OnFrameReady(m_lastJpeg);
}

void RDCCommunicatorUDP::OnFrameReady(const std::vector<BYTE>& jpeg)
{
    Log("On Frame Ready");
    
    if (m_pNotifyWnd)
    {
        m_pNotifyWnd->SetLastJpeg(jpeg);
        m_pNotifyWnd->PostMessage(WM_NEW_FRAME);
    }
}

int RDCCommunicatorUDP::ExitInstance()
{
    if (m_socket != INVALID_SOCKET)
        closesocket(m_socket);

    return CWinThread::ExitInstance();
}
