// ============================================================================
// File: ConfigManager.cpp
// Description: Implementation of configuration manager
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/ConfigManager.h"
#include "NetworkMonitor/Utils.h"
#include <shlobj.h>

namespace NetworkMonitor
{

ConfigManager::ConfigManager()
{
}

ConfigManager::~ConfigManager()
{
}

bool ConfigManager::LoadConfig(AppConfig& config)
{
    HKEY hKey = nullptr;
    if (!OpenSettingsKey(hKey))
    {
        // Use default config if cannot open registry
        config = AppConfig();
        return true;
    }

    // Load settings from registry
    config.updateInterval = ReadDWORD(hKey, L"UpdateInterval", DEFAULT_UPDATE_INTERVAL);
    config.displayUnit = static_cast<SpeedUnit>(ReadDWORD(hKey, L"DisplayUnit", static_cast<DWORD>(SpeedUnit::KiloBytesPerSecond)));
    config.showUploadSpeed = ReadDWORD(hKey, L"ShowUploadSpeed", 1) != 0;
    config.showDownloadSpeed = ReadDWORD(hKey, L"ShowDownloadSpeed", 1) != 0;
    config.enableLogging = ReadDWORD(hKey, L"EnableLogging", 1) != 0;
    config.historyAutoTrimDays = static_cast<int>(ReadDWORD(hKey, L"HistoryAutoTrimDays", DEFAULT_HISTORY_AUTO_TRIM_DAYS));
    if (config.historyAutoTrimDays > MAX_HISTORY_AUTO_TRIM_DAYS)
    {
        config.historyAutoTrimDays = MAX_HISTORY_AUTO_TRIM_DAYS;
    }
    config.selectedInterface = ReadString(hKey, L"SelectedInterface", L"");
    config.autoStart = IsAutoStartEnabled();

    RegCloseKey(hKey);
    return true;
}

bool ConfigManager::SaveConfig(const AppConfig& config)
{
    HKEY hKey = nullptr;
    if (!OpenSettingsKey(hKey))
    {
        return false;
    }

    // Save settings to registry
    bool success = true;
    success &= WriteDWORD(hKey, L"UpdateInterval", config.updateInterval);
    success &= WriteDWORD(hKey, L"DisplayUnit", static_cast<DWORD>(config.displayUnit));
    success &= WriteDWORD(hKey, L"ShowUploadSpeed", config.showUploadSpeed ? 1 : 0);
    success &= WriteDWORD(hKey, L"ShowDownloadSpeed", config.showDownloadSpeed ? 1 : 0);
    success &= WriteDWORD(hKey, L"EnableLogging", config.enableLogging ? 1 : 0);
    int trimDays = config.historyAutoTrimDays;
    if (trimDays < 0)
    {
        trimDays = 0;
    }
    else if (trimDays > MAX_HISTORY_AUTO_TRIM_DAYS)
    {
        trimDays = MAX_HISTORY_AUTO_TRIM_DAYS;
    }
    success &= WriteDWORD(hKey, L"HistoryAutoTrimDays", static_cast<DWORD>(trimDays));
    success &= WriteString(hKey, L"SelectedInterface", config.selectedInterface);

    // Save auto-start setting
    success &= SetAutoStart(config.autoStart);

    RegCloseKey(hKey);
    return success;
}

bool ConfigManager::SetAutoStart(bool enable)
{
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTOSTART_PATH, 0, KEY_WRITE, &hKey);
    
    if (result != ERROR_SUCCESS)
    {
        return false;
    }

    bool success = false;

    if (enable)
    {
        // Get executable path
        wchar_t exePath[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        // Add to startup
        result = RegSetValueExW(hKey, APP_NAME, 0, REG_SZ, 
                                reinterpret_cast<const BYTE*>(exePath), 
                                static_cast<DWORD>((wcslen(exePath) + 1) * sizeof(wchar_t)));
        
        success = (result == ERROR_SUCCESS);
    }
    else
    {
        // Remove from startup
        result = RegDeleteValueW(hKey, APP_NAME);
        success = (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
    }

    RegCloseKey(hKey);
    return success;
}

bool ConfigManager::IsAutoStartEnabled()
{
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTOSTART_PATH, 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t value[MAX_PATH] = {0};
    DWORD valueSize = sizeof(value);
    DWORD type = REG_SZ;

    result = RegQueryValueExW(hKey, APP_NAME, nullptr, &type, 
                              reinterpret_cast<BYTE*>(value), &valueSize);

    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

bool ConfigManager::OpenSettingsKey(HKEY& hKey)
{
    DWORD disposition = 0;
    LONG result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_PATH,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    return (result == ERROR_SUCCESS);
}

DWORD ConfigManager::ReadDWORD(HKEY hKey, const wchar_t* valueName, DWORD defaultValue)
{
    DWORD value = defaultValue;
    DWORD valueSize = sizeof(DWORD);
    DWORD type = REG_DWORD;

    LONG result = RegQueryValueExW(hKey, valueName, nullptr, &type, 
                                    reinterpret_cast<BYTE*>(&value), &valueSize);

    if (result != ERROR_SUCCESS || type != REG_DWORD)
    {
        return defaultValue;
    }

    return value;
}

bool ConfigManager::WriteDWORD(HKEY hKey, const wchar_t* valueName, DWORD value)
{
    LONG result = RegSetValueExW(hKey, valueName, 0, REG_DWORD, 
                                  reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));

    return (result == ERROR_SUCCESS);
}

std::wstring ConfigManager::ReadString(HKEY hKey, const wchar_t* valueName, const std::wstring& defaultValue)
{
    wchar_t buffer[256] = {0};
    DWORD bufferSize = sizeof(buffer);
    DWORD type = REG_SZ;

    LONG result = RegQueryValueExW(hKey, valueName, nullptr, &type, 
                                    reinterpret_cast<BYTE*>(buffer), &bufferSize);

    if (result != ERROR_SUCCESS || type != REG_SZ)
    {
        return defaultValue;
    }

    return std::wstring(buffer);
}

bool ConfigManager::WriteString(HKEY hKey, const wchar_t* valueName, const std::wstring& value)
{
    LONG result = RegSetValueExW(hKey, valueName, 0, REG_SZ, 
                                  reinterpret_cast<const BYTE*>(value.c_str()), 
                                  static_cast<DWORD>((value.length() + 1) * sizeof(wchar_t)));

    return (result == ERROR_SUCCESS);
}

} // namespace NetworkMonitor
