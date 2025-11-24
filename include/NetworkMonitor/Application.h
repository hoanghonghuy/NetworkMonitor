// ============================================================================
// File: Application.h
// Description: Main application class managing all components and state
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_APPLICATION_H
#define NETWORK_MONITOR_APPLICATION_H

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/ConfigManager.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/TrayIcon.h"
#include "NetworkMonitor/TaskbarOverlay.h"
#include <windows.h>
#include <memory>

namespace NetworkMonitor
{

class Application
{
public:
    Application();
    ~Application();

    // Application lifecycle
    bool Initialize(HINSTANCE hInstance);
    int Run();
    void Cleanup();

    // Component access
    HWND GetMainWindow() const { return m_hwnd; }
    HINSTANCE GetInstance() const { return m_hInstance; }
    ConfigManager* GetConfigManager() { return m_pConfigManager.get(); }
    NetworkMonitorClass* GetNetworkMonitor() { return m_pNetworkMonitor.get(); }
    TrayIcon* GetTrayIcon() { return m_pTrayIcon.get(); }
    TaskbarOverlay* GetTaskbarOverlay() { return m_pTaskbarOverlay.get(); }
    const AppConfig& GetConfig() const { return m_config; }

    // Configuration operations
    bool LoadConfig();
    bool SaveConfig();
    void ApplyLanguageFromConfig();

    // UI operations
    void ShowSettingsDialog();
    void ShowDashboardDialog();
    void ShowHistoryDialog();
    void ShowAboutDialog();
    void OnTaskbarOverlayRightClick();

    // Menu command handling
    void OnMenuCommand(UINT menuId);

    // Timer callback
    void OnTimer();

private:
    // Window procedure (static for Windows API)
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK InstanceWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Helper methods
    bool RegisterWindowClass();
    bool CreateMainWindow();
    void CenterDialogOnScreen(HWND hDlg);
    NetworkStats GetCurrentStatsForConfig();
    void LogHistorySample(const NetworkStats& stats);
    void UpdateTrayIcon(const NetworkStats& stats);
    void UpdateTaskbarOverlay(const NetworkStats& stats);

    // Component instances (using smart pointers for automatic cleanup)
    std::unique_ptr<ConfigManager> m_pConfigManager;
    std::unique_ptr<NetworkMonitorClass> m_pNetworkMonitor;
    std::unique_ptr<TrayIcon> m_pTrayIcon;
    std::unique_ptr<TaskbarOverlay> m_pTaskbarOverlay;

    // Dialog instances (only SettingsDialog is owned here for now)

    // Application state
    AppConfig m_config;
    HWND m_hwnd;
    HINSTANCE m_hInstance;

    // Previous totals for logging per-interval usage
    unsigned long long m_prevTotalBytesDown;
    unsigned long long m_prevTotalBytesUp;
    bool m_prevTotalsValid;

    // Initialization state
    bool m_initialized;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_APPLICATION_H
