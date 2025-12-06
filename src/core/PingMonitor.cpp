// ============================================================================
// File: PingMonitor.cpp
// Description: Implementation of ICMP ping monitor
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/PingMonitor.h"
#include "NetworkMonitor/Utils.h"

namespace NetworkMonitor
{

PingMonitor::PingMonitor()
    : m_hIcmp(INVALID_HANDLE_VALUE)
    , m_initialized(false)
    , m_latency(-1)
{
}

PingMonitor::~PingMonitor()
{
    Cleanup();
}

bool PingMonitor::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    m_hIcmp = IcmpCreateFile();
    if (m_hIcmp == INVALID_HANDLE_VALUE)
    {
        LogError(L"PingMonitor::Initialize: IcmpCreateFile failed");
        return false;
    }

    m_initialized = true;
    LogDebug(L"PingMonitor::Initialize: success");
    return true;
}

void PingMonitor::Cleanup()
{
    if (m_hIcmp != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(m_hIcmp);
        m_hIcmp = INVALID_HANDLE_VALUE;
    }

    m_initialized = false;
    m_latency = -1;
}

void PingMonitor::Update()
{
    if (!m_initialized || m_hIcmp == INVALID_HANDLE_VALUE)
    {
        m_latency = -1;
        return;
    }

    // Prepare reply buffer
    char replyBuffer[sizeof(ICMP_ECHO_REPLY) + 8];
    DWORD replySize = sizeof(replyBuffer);

    // Send ICMP echo request (no data payload)
    DWORD result = IcmpSendEcho(
        m_hIcmp,
        TARGET_IP,
        nullptr,    // No send data
        0,          // No send data size
        nullptr,    // No IP options
        replyBuffer,
        replySize,
        TIMEOUT_MS
    );

    if (result > 0)
    {
        PICMP_ECHO_REPLY pReply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer);
        if (pReply->Status == IP_SUCCESS)
        {
            m_latency = static_cast<int>(pReply->RoundTripTime);
        }
        else
        {
            m_latency = -1;
        }
    }
    else
    {
        m_latency = -1;
    }
}

} // namespace NetworkMonitor
