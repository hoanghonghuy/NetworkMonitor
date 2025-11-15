// ============================================================================
// File: HistoryLogger.h
// Description: Logging of network usage history to SQLite database
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_HISTORYLOGGER_H
#define NETWORK_MONITOR_HISTORYLOGGER_H

#include "NetworkMonitor/Common.h"
#include <string>
#include <vector>
#include <ctime>

struct sqlite3;

namespace NetworkMonitor
{

struct HistorySample
{
    std::time_t timestamp;           // UTC timestamp (seconds since epoch)
    std::wstring interfaceName;      // Interface name or "All Interfaces"
    unsigned long long bytesDown;    // Bytes downloaded in interval
    unsigned long long bytesUp;      // Bytes uploaded in interval
};

class HistoryLogger
{
public:
    static HistoryLogger& Instance();

    /**
     * Append a usage sample (delta bytes for the interval).
     * If SQLite is not available, the call is a no-op.
     */
    void AppendSample(const std::wstring& interfaceName,
                      unsigned long long bytesDown,
                      unsigned long long bytesUp);

    // These will be implemented later for the dashboard
    bool GetTotalsToday(unsigned long long& totalDown, unsigned long long& totalUp);
    bool GetTotalsThisMonth(unsigned long long& totalDown, unsigned long long& totalUp);
    bool GetRecentSamples(int limit, std::vector<HistorySample>& outSamples);

private:
    HistoryLogger();
    ~HistoryLogger();

    HistoryLogger(const HistoryLogger&) = delete;
    HistoryLogger& operator=(const HistoryLogger&) = delete;

    void EnsureInitialized();
    void InitializeSQLite();
    void ShutdownSQLite();

    bool InsertSampleSQLite(std::time_t ts,
                            const std::wstring& iface,
                            unsigned long long down,
                            unsigned long long up);

    bool m_initialized;
    bool m_sqliteAvailable;

    sqlite3* m_db;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_HISTORYLOGGER_H
