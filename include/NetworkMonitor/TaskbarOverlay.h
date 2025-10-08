// ============================================================================
// File: TaskbarOverlay.h
// Description: Taskbar overlay window for displaying network speed
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_TASKBAROVERLAY_H
#define NETWORK_MONITOR_TASKBAROVERLAY_H

#include "NetworkMonitor/Common.h"
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <functional>

namespace NetworkMonitor
{

class TaskbarOverlay
{
public:
    TaskbarOverlay();
    ~TaskbarOverlay();

    /**
     * Initialize and create taskbar overlay window
     * @param hInstance Application instance handle
     * @return true if successful, false otherwise
     */
    bool Initialize(HINSTANCE hInstance);

    /**
     * Cleanup and destroy overlay window
     */
    void Cleanup();

    /**
     * Update displayed network speed
     * @param downloadSpeed Download speed in bytes/sec
     * @param uploadSpeed Upload speed in bytes/sec
     * @param unit Display unit for speed
     */
    void UpdateSpeed(double downloadSpeed, double uploadSpeed, SpeedUnit unit);

    /**
     * Show or hide overlay window
     * @param show true to show, false to hide
     */
    void Show(bool show);

    /**
     * Check if overlay is visible
     * @return true if visible, false otherwise
     */
    bool IsVisible() const { return m_isVisible; }

    /**
     * Set callback for right-click event
     * @param callback Callback function
     */
    void SetRightClickCallback(std::function<void()> callback);

private:
    /**
     * Register window class
     * @param hInstance Application instance
     * @return true if successful
     */
    bool RegisterWindowClass(HINSTANCE hInstance);

    /**
     * Create overlay window
     * @param hInstance Application instance
     * @return true if successful
     */
    bool CreateOverlayWindow(HINSTANCE hInstance);

    /**
     * Position window on taskbar
     */
    void PositionOnTaskbar();

    /**
     * Get taskbar information
     * @param rect Output taskbar rectangle
     * @param edge Output taskbar edge (ABE_BOTTOM, ABE_TOP, etc.)
     * @return true if successful
     */
    bool GetTaskbarInfo(RECT& rect, UINT& edge);

    /**
     * Window procedure for overlay window
     */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /**
     * Handle paint message
     */
    void OnPaint();

    /**
     * Handle right button up message
     */
    void OnRightButtonUp();

    /**
     * Handle display change message
     */
    void OnDisplayChange();

    /**
     * Force show window if hidden
     */
    void ForceShow();

private:
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"NetworkMonitorTaskbarOverlay";
    static constexpr int WINDOW_WIDTH = 95;
    static constexpr int WINDOW_HEIGHT = 36;
    static constexpr int TASKBAR_MARGIN = 5;
    static constexpr UINT TIMER_CHECK_VISIBILITY = 1001;

    HINSTANCE m_hInstance;                      // Application instance
    HWND m_hwnd;                                // Window handle
    HWND m_hTaskbar;                            // Taskbar window handle
    bool m_isVisible;                           // Is window visible
    bool m_initialized;                         // Is initialized
    UINT_PTR m_timerId;                         // Timer ID for visibility check

    // Display data
    std::wstring m_speedText;                   // Current speed text to display
    double m_downloadSpeed;                     // Current download speed
    double m_uploadSpeed;                       // Current upload speed
    SpeedUnit m_displayUnit;                    // Display unit

    // Callback
    std::function<void()> m_rightClickCallback; // Right-click callback
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_TASKBAROVERLAY_H
