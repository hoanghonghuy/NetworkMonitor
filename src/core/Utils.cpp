// ============================================================================
// File: Utils.cpp
// Description: Implementation of utility functions
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/Utils.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <shellapi.h>

namespace NetworkMonitor
{

static bool g_debugLoggingEnabled = false;

// ============================================================================
// STRING UTILITIES IMPLEMENTATION
// ============================================================================

std::wstring FormatSpeed(double bytesPerSecond, SpeedUnit unit)
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;

    double convertedValue = 0.0;
    std::wstring unitStr;

    switch (unit)
    {
    case SpeedUnit::BytesPerSecond:
    {
        convertedValue = bytesPerSecond;
        unitStr = L"B/s";
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"KB/s";
        }
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"MB/s";
        }
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"GB/s";
        }
        break;
    }

    case SpeedUnit::KiloBytesPerSecond:
    {
        convertedValue = bytesPerSecond / KB;
        unitStr = L"KB/s";
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"MB/s";
        }
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"GB/s";
        }
        break;
    }

    case SpeedUnit::MegaBytesPerSecond:
    {
        convertedValue = bytesPerSecond / MB;
        unitStr = L"MB/s";
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"GB/s";
        }
        break;
    }

    case SpeedUnit::MegaBitsPerSecond:
    default:
        convertedValue = (bytesPerSecond * 8.0) / 1000000.0; // decimal Mbps
        unitStr = L"Mbps";
        break;
    }

    std::wostringstream oss;
    oss << std::fixed << std::setprecision(2) << convertedValue << L" " << unitStr;
    return oss.str();
}

std::wstring FormatBytes(ULONG64 bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    std::wostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (bytes >= TB)
    {
        oss << (bytes / TB) << L" TB";
    }
    else if (bytes >= GB)
    {
        oss << (bytes / GB) << L" GB";
    }
    else if (bytes >= MB)
    {
        oss << (bytes / MB) << L" MB";
    }
    else if (bytes >= KB)
    {
        oss << (bytes / KB) << L" KB";
    }
    else
    {
        oss << bytes << L" B";
    }

    return oss.str();
}

std::wstring SpeedUnitToString(SpeedUnit unit)
{
    switch (unit)
    {
    case SpeedUnit::BytesPerSecond:
        return L"B/s";
    case SpeedUnit::KiloBytesPerSecond:
        return L"KB/s";
    case SpeedUnit::MegaBytesPerSecond:
        return L"MB/s";
    case SpeedUnit::MegaBitsPerSecond:
        return L"Mbps";
    default:
        return L"KB/s";
    }
}

std::wstring LoadStringResource(UINT resourceId)
{
    wchar_t buffer[256] = {};

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    int length = 0;
    if (hInstance)
    {
        length = LoadStringW(hInstance, resourceId, buffer, static_cast<int>(sizeof(buffer) / sizeof(wchar_t)));
    }

    if (length <= 0)
    {
        return std::wstring();
    }

    return std::wstring(buffer, length);
}

// ============================================================================
// CONVERSION UTILITIES IMPLEMENTATION
// ============================================================================

double ConvertSpeed(double bytesPerSecond, SpeedUnit unit)
{
    switch (unit)
    {
    case SpeedUnit::BytesPerSecond:
        return bytesPerSecond;

    case SpeedUnit::KiloBytesPerSecond:
        return bytesPerSecond / 1024.0;

    case SpeedUnit::MegaBytesPerSecond:
        return bytesPerSecond / (1024.0 * 1024.0);

    case SpeedUnit::MegaBitsPerSecond:
        return (bytesPerSecond * 8.0) / (1000.0 * 1000.0); // 1 Mbps = 1,000,000 bits

    default:
        return bytesPerSecond / 1024.0; // Default to KB/s
    }
}

// ============================================================================
// TIME UTILITIES IMPLEMENTATION
// ============================================================================

double GetElapsedSeconds(DWORD start, DWORD end)
{
    // Handle GetTickCount wraparound (occurs every ~49.7 days)
    DWORD elapsed;
    if (end >= start)
    {
        elapsed = end - start;
    }
    else
    {
        // Wraparound occurred
        elapsed = (MAXDWORD - start) + end + 1;
    }

    return elapsed / 1000.0; // Convert milliseconds to seconds
}

// ============================================================================
// ERROR HANDLING UTILITIES IMPLEMENTATION
// ============================================================================

std::wstring GetLastErrorString()
{
    DWORD errorCode = GetLastError();
    if (errorCode == 0)
    {
        return L"No error";
    }

    LPWSTR buffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );

    std::wstring message;
    if (size > 0 && buffer != nullptr)
    {
        message = buffer;
        LocalFree(buffer);
    }
    else
    {
        message = L"Unknown error";
    }

    return message;
}

void ShowErrorMessage(const std::wstring& message, const std::wstring& title)
{
    LogError(title + L": " + message);
    MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

namespace
{
    std::wstring GetLogFilePath()
    {
        wchar_t buffer[MAX_PATH] = {0};
        DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, static_cast<DWORD>(MAX_PATH));

        std::wstring basePath;
        if (length == 0 || length >= MAX_PATH)
        {
            basePath = L".";
        }
        else
        {
            basePath.assign(buffer, length);
        }

        std::wstring dirPath = basePath + L"\\NetworkMonitor";
        CreateDirectoryW(dirPath.c_str(), nullptr);

        std::wstring filePath = dirPath + L"\\NetworkMonitor.log";
        return filePath;
    }

    void AppendLogLine(const wchar_t* level, const std::wstring& message)
    {
        if (!level)
        {
            return;
        }

        SYSTEMTIME st = {};
        GetLocalTime(&st);

        wchar_t timeBuffer[32] = {0};
        swprintf_s(timeBuffer, L"%04u-%02u-%02u %02u:%02u:%02u",
                   st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        std::wstring filePath = GetLogFilePath();

        try
        {
            std::wofstream file(filePath, std::ios::app);
            if (!file.is_open())
            {
                return;
            }

            file << timeBuffer << L" [" << level << L"] " << message << L"\n";
        }
        catch (...)
        {
        }
    }
}

void LogDebug(const std::wstring& message)
{
    if (!g_debugLoggingEnabled)
    {
        return;
    }

    AppendLogLine(L"DEBUG", message);
}

void LogError(const std::wstring& message)
{
    AppendLogLine(L"ERROR", message);
}

void SetDebugLoggingEnabled(bool enabled)
{
    g_debugLoggingEnabled = enabled;
}

void OpenLogFileInExplorer()
{
    std::wstring logPath = GetLogFilePath();

    if (logPath.empty())
    {
        return;
    }

    if (GetFileAttributesW(logPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        ShellExecuteW(nullptr, L"open", logPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    size_t pos = logPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
    {
        std::wstring folder = logPath.substr(0, pos);
        ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

// ============================================================================
// UI UTILITIES IMPLEMENTATION
// ============================================================================

void CenterWindowOnScreen(HWND hWnd)
{
    if (!hWnd)
    {
        return;
    }

    RECT rc = {0};
    if (!GetWindowRect(hWnd, &rc))
    {
        return;
    }

    int dlgWidth = rc.right - rc.left;
    int dlgHeight = rc.bottom - rc.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int posX = (screenWidth - dlgWidth) / 2;
    int posY = (screenHeight - dlgHeight) / 2;

    SetWindowPos(hWnd, nullptr, posX, posY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

} // namespace NetworkMonitor
