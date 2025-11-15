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

bool HistoryLogger::GetTotalsToday(unsigned long long& totalDown, unsigned long long& totalUp)
{
    totalDown = 0;
    totalUp = 0;

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    if (localtime_s(&localTime, &now) != 0)
    {
        return false;
    }

    localTime.tm_hour = 0;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;

    std::time_t start = std::mktime(&localTime);
    if (start == static_cast<std::time_t>(-1))
    {
        return false;
    }

    std::time_t end = start + 24 * 60 * 60;

    const char* sql =
        "SELECT COALESCE(SUM(bytes_down), 0), COALESCE(SUM(bytes_up), 0) "
        "FROM usage WHERE timestamp >= ? AND timestamp < ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(start));
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(end));

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        totalDown = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 0));
        totalUp = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 1));
    }

    sqlite3_finalize(stmt);

    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool HistoryLogger::GetTotalsThisMonth(unsigned long long& totalDown, unsigned long long& totalUp)
{
    totalDown = 0;
    totalUp = 0;

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::tm startTm = {};
    if (localtime_s(&startTm, &now) != 0)
    {
        return false;
    }

    startTm.tm_mday = 1;
    startTm.tm_hour = 0;
    startTm.tm_min = 0;
    startTm.tm_sec = 0;

    std::time_t start = std::mktime(&startTm);
    if (start == static_cast<std::time_t>(-1))
    {
        return false;
    }

    std::tm endTm = startTm;
    endTm.tm_mon += 1;
    if (endTm.tm_mon >= 12)
    {
        endTm.tm_mon -= 12;
        endTm.tm_year += 1;
    }

    std::time_t end = std::mktime(&endTm);
    if (end == static_cast<std::time_t>(-1))
    {
        return false;
    }

    const char* sql =
        "SELECT COALESCE(SUM(bytes_down), 0), COALESCE(SUM(bytes_up), 0) "
        "FROM usage WHERE timestamp >= ? AND timestamp < ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(start));
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(end));

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        totalDown = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 0));
        totalUp = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 1));
    }

    sqlite3_finalize(stmt);

    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool HistoryLogger::GetRecentSamples(int limit, std::vector<HistorySample>& outSamples)
{
    outSamples.clear();

    if (limit <= 0)
    {
        return true;
    }

    EnsureInitialized();
    if (!m_sqliteAvailable || !m_db)
    {
        return false;
    }

    const char* sql =
        "SELECT timestamp, interface, bytes_down, bytes_up "
        "FROM usage "
        "ORDER BY timestamp DESC "
        "LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        HistorySample sample;
        sample.timestamp = static_cast<std::time_t>(sqlite3_column_int64(stmt, 0));

        const void* ifaceText = sqlite3_column_text16(stmt, 1);
        if (ifaceText)
        {
            sample.interfaceName.assign(static_cast<const wchar_t*>(ifaceText));
        }
        else
        {
            sample.interfaceName.clear();
        }

        sample.bytesDown = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 2));
        sample.bytesUp = static_cast<unsigned long long>(sqlite3_column_int64(stmt, 3));

        outSamples.push_back(std::move(sample));
    }

    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

} // namespace NetworkMonitor
