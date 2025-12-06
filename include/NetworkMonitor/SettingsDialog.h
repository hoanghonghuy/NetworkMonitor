// ============================================================================
// File: SettingsDialog.h
// Description: Settings dialog management for NetworkMonitor
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_SETTINGS_DIALOG_H
#define NETWORK_MONITOR_SETTINGS_DIALOG_H

#include "NetworkMonitor/Common.h"
#include <functional>

namespace NetworkMonitor
{

class ConfigManager;
class NetworkMonitorClass;

class SettingsDialog
{x
public:
    SettingsDialog();
    ~SettingsDialog();

    // Show the settings dialog modally
    bool Show(HWND parentWindow, ConfigManager* configManager, NetworkMonitorClass* networkMonitor);

    // Set callback for when settings are applied
    void SetSettingsChangedCallback(std::function<void()> callback);

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    void PopulateDialog(HWND hDlg);
    bool ApplySettingsFromDialog(HWND hDlg);
    void PopulateInterfaceCombo(HWND hDlg);
    void CenterDialogOnScreen(HWND hDlg);

    // Member variables
    HWND m_hDialog;
    ConfigManager* m_pConfigManager;
    NetworkMonitorClass* m_pNetworkMonitor;
    AppConfig m_configCopy;  // Working copy of config
    std::function<void()> m_settingsChangedCallback;
    bool m_isInitializing;   // Prevent recursive updates during initialization
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_SETTINGS_DIALOG_H
