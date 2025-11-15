// ============================================================================
// File: TrayIcon.h
// Description: System tray icon management and user interaction
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_TRAYICON_H
#define NETWORK_MONITOR_TRAYICON_H

#include "NetworkMonitor/Common.h"
#include <windows.h>
#include <shellapi.h>
#include <functional>

#pragma comment(lib, "shell32.lib")

namespace NetworkMonitor
{

class TrayIcon
{
public:
    TrayIcon();
    ~TrayIcon();

    /**
     * Initialize and create tray icon
     * @param hwnd Parent window handle
     * @return true if successful, false otherwise
     */
    bool Initialize(HWND hwnd);

    /**
     * Cleanup and remove tray icon
     */
    void Cleanup();

    /**
     * Update tray icon tooltip with network statistics
     * @param stats Network statistics to display
     * @param unit Display unit for speed
     */
    void UpdateTooltip(const NetworkStats& stats, SpeedUnit unit);

    /**
     * Update tray icon based on traffic activity
     * @param downloadSpeed Current download speed in bytes/sec
     * @param uploadSpeed Current upload speed in bytes/sec
     */
    void UpdateIcon(double downloadSpeed, double uploadSpeed);

    /**
     * Handle tray icon messages
     * @param message Message ID
     * @param wParam WPARAM parameter
     * @param lParam LPARAM parameter
     * @return true if message handled, false otherwise
     */
    bool HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    /**
     * Show context menu at cursor position
     */
    void ShowContextMenu();

    /**
     * Set callback for menu item selection
     * @param callback Callback function receiving menu item ID
     */
    void SetMenuCallback(std::function<void(UINT)> callback);

    /**
     * Provide pointer to current configuration (for reflecting menu state)
     */
    void SetConfigSource(const AppConfig* config);

    /**
     * Provide callback to query taskbar overlay visibility state
     */
    void SetOverlayVisibilityProvider(std::function<bool()> provider);

private:
    /**
     * Create context menu
     * @param config Current application configuration
     * @return Menu handle
     */
    HMENU CreateContextMenu(const AppConfig& config, bool overlayVisible);

    /**
     * Load application icon
     * @return Icon handle
     */
    HICON LoadAppIcon();

private:
    HWND m_hwnd;                                    // Parent window handle
    NOTIFYICONDATAW m_notifyIconData;               // Notify icon data structure
    bool m_initialized;                             // Is initialized?
    HICON m_iconIdle;                               // Icon when idle
    HICON m_iconActive;                             // Icon when active
    HICON m_iconHigh;                               // Icon when high traffic
    std::function<void(UINT)> m_menuCallback;       // Menu selection callback
    const AppConfig* m_configRef;                   // Current config reference
    std::function<bool()> m_overlayVisibleProvider; // Overlay visibility provider
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_TRAYICON_H
