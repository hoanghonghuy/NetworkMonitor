// ============================================================================
// File: ConfigManager.h
// Description: Configuration manager for saving/loading application settings
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_CONFIGMANAGER_H
#define NETWORK_MONITOR_CONFIGMANAGER_H

#include "NetworkMonitor/Common.h"
#include <windows.h>

namespace NetworkMonitor {

class ConfigManager {
public:
  ConfigManager();
  ~ConfigManager();

  /**
   * Load configuration from registry
   * @param config Output configuration
   * @return true if successful, false otherwise
   */
  bool LoadConfig(AppConfig &config);

  /**
   * Save configuration to registry
   * @param config Configuration to save
   * @return true if successful, false otherwise
   */
  bool SaveConfig(const AppConfig &config);

  /**
   * Enable/disable auto-start with Windows
   * @param enable true to enable, false to disable
   * @return true if successful, false otherwise
   */
  bool SetAutoStart(bool enable);

  /**
   * Check if auto-start is enabled
   * @return true if enabled, false otherwise
   */
  bool IsAutoStartEnabled();

private:
  /**
   * Open or create registry key for application settings
   * @param hKey Output key handle
   * @return true if successful, false otherwise
   */
  bool OpenSettingsKey(HKEY &hKey);

  /**
   * Read DWORD value from registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param defaultValue Default value if not found
   * @return Value read from registry or default
   */
  DWORD ReadDWORD(HKEY hKey, const wchar_t *valueName, DWORD defaultValue);

  /**
   * Write DWORD value to registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param value Value to write
   * @return true if successful, false otherwise
   */
  bool WriteDWORD(HKEY hKey, const wchar_t *valueName, DWORD value);

  /**
   * Read string value from registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param defaultValue Default value if not found
   * @return Value read from registry or default
   */
  std::wstring ReadString(HKEY hKey, const wchar_t *valueName,
                          const std::wstring &defaultValue);

  /**
   * Write string value to registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param value Value to write
   * @return true if successful, false otherwise
   */
  bool WriteString(HKEY hKey, const wchar_t *valueName,
                   const std::wstring &value);

private:
  // FIX: Thêm const vào đây
  static constexpr const wchar_t *REGISTRY_PATH = L"Software\\NetworkMonitor";
  static constexpr const wchar_t *AUTOSTART_PATH =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_CONFIGMANAGER_H
