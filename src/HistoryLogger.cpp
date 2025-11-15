// ============================================================================
// File: HistoryLogger.cpp
// Description: Implementation of network usage history logger using SQLite
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/HistoryLogger.h"

#include <cwchar>   // wcsrchr
#include <ctime>
#include "sqlite3.h"

namespace NetworkMonitor
{

HistoryLogger& HistoryLogger::Instance()
{
    static HistoryLogger instance;
    return instance;
}

HistoryLogger::HistoryLogger()
    : m_initialized(false)
    , m_sqliteAvailable(false)
    , m_db(nullptr)
{
}

HistoryLogger::~HistoryLogger()
{
    ShutdownSQLite();
}

void HistoryLogger::EnsureInitialized()
{
    if (m_initialized)
    {
        return;
    }

    m_initialized = true;
    InitializeSQLite();
}

void HistoryLogger::InitializeSQLite()
{
    m_sqliteAvailable = false;

    // Build database path next to the executable
    wchar_t exePath[MAX_PATH] = {0};
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
    {
        ShutdownSQLite();
        return;
    }

    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash)
    {
        *lastSlash = L'\0';
    }

    wchar_t dbPath[MAX_PATH] = {0};
    swprintf_s(dbPath, L"%s\\network_usage.db", exePath);

    if (sqlite3_open16(dbPath, &m_db) != SQLITE_OK || !m_db)
    {
        return;
    }

    // Create table and index if they don't exist yet
    const char* createSql =
        "CREATE TABLE IF NOT EXISTS usage ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp INTEGER NOT NULL,"
        "interface TEXT NOT NULL,"
        "bytes_down INTEGER NOT NULL,"
        "bytes_up INTEGER NOT NULL);"
        "CREATE INDEX IF NOT EXISTS idx_usage_ts ON usage(timestamp);";

    sqlite3_exec(m_db, createSql, nullptr, nullptr, nullptr);

    m_sqliteAvailable = true;
}

void HistoryLogger::ShutdownSQLite()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    m_sqliteAvailable = false;
}

void HistoryLogger::AppendSample(const std::wstring& interfaceName,
                                 unsigned long long bytesDown,
                                 unsigned long long bytesUp)
{
    if (bytesDown == 0 && bytesUp == 0)
    {
        return;
    }

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return;
    }

    std::time_t now = std::time(nullptr);
    InsertSampleSQLite(now, interfaceName, bytesDown, bytesUp);
}

bool HistoryLogger::InsertSampleSQLite(std::time_t ts,
                                       const std::wstring& iface,
                                       unsigned long long down,
                                       unsigned long long up)
{
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    static const wchar_t* INSERT_SQL =
        L"INSERT INTO usage (timestamp, interface, bytes_down, bytes_up) "
        L"VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare16_v2(m_db, INSERT_SQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(ts));
    sqlite3_bind_text16(stmt, 2, iface.c_str(), -1, nullptr);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(down));
    sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(up));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE || rc == SQLITE_OK);
}

// Dashboard-related queries will be implemented in a later step
bool HistoryLogger::GetTotalsToday(unsigned long long& totalDown, unsigned long long& totalUp)
{
    totalDown = 0;
    totalUp = 0;
    return false;
}

bool HistoryLogger::GetTotalsThisMonth(unsigned long long& totalDown, unsigned long long& totalUp)
{
    totalDown = 0;
    totalUp = 0;
    return false;
}

bool HistoryLogger::GetRecentSamples(int /*limit*/, std::vector<HistorySample>& outSamples)
{
    outSamples.clear();
    return false;
}

} // namespace NetworkMonitor
