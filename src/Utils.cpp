// ============================================================================
// File: Utils.cpp
// Description: Implementation of utility functions
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/Utils.h"
#include <sstream>
#include <iomanip>

namespace NetworkMonitor
{

// ============================================================================
// STRING UTILITIES IMPLEMENTATION
// ============================================================================

std::wstring FormatSpeed(double bytesPerSecond, SpeedUnit unit)
{
    double convertedValue = ConvertSpeed(bytesPerSecond, unit);
    std::wstring unitStr = SpeedUnitToString(unit);

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
    MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

} // namespace NetworkMonitor
