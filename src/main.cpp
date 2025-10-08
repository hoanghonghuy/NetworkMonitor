// ============================================================================
// File: main.cpp
// Description: Application entry point and main message loop
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/TrayIcon.h"
#include "NetworkMonitor/ConfigManager.h"
#include "NetworkMonitor/TaskbarOverlay.h"
#include "NetworkMonitor/Utils.h"
#include <windows.h>

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

NetworkMonitor::NetworkMonitorClass* g_pNetworkMonitor = nullptr;
NetworkMonitor::TrayIcon* g_pTrayIcon = nullptr;
NetworkMonitor::ConfigManager* g_pConfigManager = nullptr;
NetworkMonitor::TaskbarOverlay* g_pTaskbarOverlay = nullptr;  // THÊM MỚI
NetworkMonitor::AppConfig g_config;
HWND g_hwnd = nullptr;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool InitializeApplication(HINSTANCE hInstance);
void CleanupApplication();
void OnTimer();
void OnMenuCommand(UINT menuId);
void ShowAboutDialog(HWND hwnd);
void OnTaskbarOverlayRightClick();  // THÊM MỚI

// ============================================================================
// WINMAIN - APPLICATION ENTRY POINT
// ============================================================================

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Check if another instance is already running
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"NetworkMonitor_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(nullptr, L"NetworkMonitor is already running!", APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // Initialize application
    if (!InitializeApplication(hInstance))
    {
        CleanupApplication();
        if (hMutex)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return -1;
    }

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    CleanupApplication();

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return static_cast<int>(msg.wParam);
}

// ============================================================================
// WINDOW PROCEDURE
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            // Start timer for network monitoring updates
            SetTimer(hwnd, TIMER_UPDATE_NETWORK, g_config.updateInterval, nullptr);
            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == TIMER_UPDATE_NETWORK)
            {
                OnTimer();
            }
            return 0;
        }

        case WM_TRAYICON:
        {
            // Handle tray icon messages
            if (g_pTrayIcon)
            {
                g_pTrayIcon->HandleMessage(message, wParam, lParam);
            }
            return 0;
        }

        case WM_COMMAND:
        {
            // Handle menu commands
            OnMenuCommand(LOWORD(wParam));
            return 0;
        }

        case WM_DESTROY:
        {
            // Kill timer
            KillTimer(hwnd, TIMER_UPDATE_NETWORK);
            
            // Post quit message
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool InitializeApplication(HINSTANCE hInstance)
{
    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_WINDOW_CLASS;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&wc))
    {
        NetworkMonitor::ShowErrorMessage(L"Failed to register window class");
        return false;
    }

    // Create hidden window (for message processing only)
    g_hwnd = CreateWindowExW(
        0,
        APP_WINDOW_CLASS,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hwnd)
    {
        NetworkMonitor::ShowErrorMessage(L"Failed to create window");
        return false;
    }

    // Do not show window (it's hidden)
    // ShowWindow(g_hwnd, SW_HIDE);

    // Create config manager
    g_pConfigManager = new NetworkMonitor::ConfigManager();
    if (!g_pConfigManager->LoadConfig(g_config))
    {
        // Use default config if load fails
        g_config = NetworkMonitor::AppConfig();
    }

    // Create tray icon
    g_pTrayIcon = new NetworkMonitor::TrayIcon();
    if (!g_pTrayIcon->Initialize(g_hwnd))
    {
        NetworkMonitor::ShowErrorMessage(L"Failed to initialize tray icon");
        return false;
    }

    // Set menu callback
    g_pTrayIcon->SetMenuCallback(OnMenuCommand);

    // THÊM MỚI: Create taskbar overlay
    g_pTaskbarOverlay = new NetworkMonitor::TaskbarOverlay();
    if (!g_pTaskbarOverlay->Initialize(hInstance))
    {
        NetworkMonitor::ShowErrorMessage(L"Failed to initialize taskbar overlay");
        // Don't fail completely, just log warning
        SAFE_DELETE(g_pTaskbarOverlay);
    }
    else
    {
        // Set right-click callback for overlay
        g_pTaskbarOverlay->SetRightClickCallback(OnTaskbarOverlayRightClick);
        
        // Show overlay by default
        g_pTaskbarOverlay->Show(true);
    }

    // Create network monitor
    g_pNetworkMonitor = new NetworkMonitor::NetworkMonitorClass();
    if (!g_pNetworkMonitor->Start())
    {
        NetworkMonitor::ShowErrorMessage(L"Failed to start network monitoring");
        return false;
    }

    return true;
}

// ============================================================================
// CLEANUP
// ============================================================================

void CleanupApplication()
{
    // Stop network monitoring
    if (g_pNetworkMonitor)
    {
        g_pNetworkMonitor->Stop();
        SAFE_DELETE(g_pNetworkMonitor);
    }

    // Cleanup taskbar overlay
    if (g_pTaskbarOverlay)
    {
        g_pTaskbarOverlay->Cleanup();
        SAFE_DELETE(g_pTaskbarOverlay);
    }

    // Cleanup tray icon
    if (g_pTrayIcon)
    {
        g_pTrayIcon->Cleanup();
        SAFE_DELETE(g_pTrayIcon);
    }

    // Cleanup config manager
    SAFE_DELETE(g_pConfigManager);
}

// ============================================================================
// TIMER CALLBACK
// ============================================================================

void OnTimer()
{
    if (!g_pNetworkMonitor)
    {
        return;
    }

    // Update network statistics
    g_pNetworkMonitor->Update();

    // Get aggregated stats
    NetworkMonitor::NetworkStats stats = g_pNetworkMonitor->GetAggregatedStats();

    // Update tray icon
    if (g_pTrayIcon)
    {
        g_pTrayIcon->UpdateTooltip(stats, g_config.displayUnit);
        g_pTrayIcon->UpdateIcon(stats.currentDownloadSpeed, stats.currentUploadSpeed);
    }

    // THÊM MỚI: Update taskbar overlay
    if (g_pTaskbarOverlay && g_pTaskbarOverlay->IsVisible())
    {
        g_pTaskbarOverlay->UpdateSpeed(
            stats.currentDownloadSpeed,
            stats.currentUploadSpeed,
            g_config.displayUnit
        );
    }
}

// ============================================================================
// MENU COMMAND HANDLER
// ============================================================================

void OnMenuCommand(UINT menuId)
{
    switch (menuId)
    {
        case IDM_UPDATE_FAST:
            g_config.updateInterval = UPDATE_INTERVAL_FAST;
            g_pConfigManager->SaveConfig(g_config);
            KillTimer(g_hwnd, TIMER_UPDATE_NETWORK);
            SetTimer(g_hwnd, TIMER_UPDATE_NETWORK, g_config.updateInterval, nullptr);
            break;

        case IDM_UPDATE_NORMAL:
            g_config.updateInterval = UPDATE_INTERVAL_NORMAL;
            g_pConfigManager->SaveConfig(g_config);
            KillTimer(g_hwnd, TIMER_UPDATE_NETWORK);
            SetTimer(g_hwnd, TIMER_UPDATE_NETWORK, g_config.updateInterval, nullptr);
            break;

        case IDM_UPDATE_SLOW:
            g_config.updateInterval = UPDATE_INTERVAL_SLOW;
            g_pConfigManager->SaveConfig(g_config);
            KillTimer(g_hwnd, TIMER_UPDATE_NETWORK);
            SetTimer(g_hwnd, TIMER_UPDATE_NETWORK, g_config.updateInterval, nullptr);
            break;

        case IDM_AUTOSTART:
            g_config.autoStart = !g_config.autoStart;
            g_pConfigManager->SaveConfig(g_config);
            break;

        // THÊM MỚI: Toggle taskbar overlay
        case IDM_SHOW_TASKBAR_OVERLAY:
            if (g_pTaskbarOverlay)
            {
                bool isVisible = g_pTaskbarOverlay->IsVisible();
                g_pTaskbarOverlay->Show(!isVisible);
            }
            break;

        case IDM_SETTINGS:
            MessageBoxW(g_hwnd, L"Settings dialog not implemented yet", APP_NAME, MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_ABOUT:
            ShowAboutDialog(g_hwnd);
            break;

        case IDM_EXIT:
            DestroyWindow(g_hwnd);
            break;
    }
}

// ============================================================================
// TASKBAR OVERLAY RIGHT-CLICK HANDLER
// ============================================================================

void OnTaskbarOverlayRightClick()
{
    // When user right-clicks on taskbar overlay, show tray icon menu
    if (g_pTrayIcon)
    {
        g_pTrayIcon->ShowContextMenu(g_config);
    }
}

// ============================================================================
// ABOUT DIALOG
// ============================================================================

void ShowAboutDialog(HWND hwnd)
{
    std::wstring message = APP_NAME;
    message += L"\n";
    message += L"Version: ";
    message += APP_VERSION;
    message += L"\n\n";
    message += L"A lightweight network traffic monitor for Windows.\n";
    message += L"Displays real-time download/upload speeds in the system tray and taskbar.";

    MessageBoxW(hwnd, message.c_str(), L"About NetworkMonitor", MB_OK | MB_ICONINFORMATION);
}
