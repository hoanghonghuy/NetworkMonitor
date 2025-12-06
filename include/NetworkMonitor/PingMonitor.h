// ============================================================================
// File: PingMonitor.h
// Description: Lightweight ICMP ping monitor using Windows API
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_PING_MONITOR_H
#define NETWORK_MONITOR_PING_MONITOR_H

#include "NetworkMonitor/Common.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#pragma comment(lib, "iphlpapi.lib")

namespace NetworkMonitor
{

class PingMonitor
{
public:
    PingMonitor();
    ~PingMonitor();

    bool Initialize();
    void Cleanup();

    // Perform ping and update latency (call from timer)
    void Update();

    // Get last measured latency in milliseconds (-1 if failed/timeout)
    int GetLatency() const { return m_latency; }

    // Check if ping is available
    bool IsAvailable() const { return m_initialized; }

private:
    HANDLE m_hIcmp;
    bool m_initialized;
    int m_latency;  // -1 = unavailable/timeout

    // Target IP (Google DNS)
    static constexpr ULONG TARGET_IP = 0x08080808;  // 8.8.8.8 in network byte order
    static constexpr DWORD TIMEOUT_MS = 1000;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_PING_MONITOR_H
