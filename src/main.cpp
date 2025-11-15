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
#include "NetworkMonitor/HistoryLogger.h"
#include "../resources/resource.h"
#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <limits>

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

NetworkMonitor::NetworkMonitorClass* g_pNetworkMonitor = nullptr;
NetworkMonitor::TrayIcon* g_pTrayIcon = nullptr;
NetworkMonitor::ConfigManager* g_pConfigManager = nullptr;
NetworkMonitor::TaskbarOverlay* g_pTaskbarOverlay = nullptr;  // THÊM MỚI
NetworkMonitor::AppConfig g_config;
HWND g_hwnd = nullptr;
HINSTANCE g_hInstance = nullptr;

// Previous totals for logging per-interval usage
unsigned long long g_prevTotalBytesDown = 0;
unsigned long long g_prevTotalBytesUp = 0;
bool g_prevTotalsValid = false;

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
void ShowSettingsDialog(HWND parent);
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void PopulateSettingsDialog(HWND hDlg);
bool ApplySettingsFromDialog(HWND hDlg);
void PopulateInterfaceCombo(HWND hDlg);
NetworkMonitor::NetworkStats GetSelectedNetworkStats();
void CenterDialogOnScreen(HWND hDlg);

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

    g_hInstance = hInstance;

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

void CenterDialogOnScreen(HWND hDlg)
{
    RECT rc = {0};
    if (!GetWindowRect(hDlg, &rc))
    {
        return;
    }

    int dlgWidth = rc.right - rc.left;
    int dlgHeight = rc.bottom - rc.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int posX = (screenWidth - dlgWidth) / 2;
    int posY = (screenHeight - dlgHeight) / 2;

    SetWindowPos(hDlg, nullptr, posX, posY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
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
    g_pTrayIcon->SetConfigSource(&g_config);

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

    g_pTrayIcon->SetOverlayVisibilityProvider([]() -> bool {
        return g_pTaskbarOverlay != nullptr && g_pTaskbarOverlay->IsVisible();
    });

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

    NetworkMonitor::NetworkStats stats;
    bool useSpecificInterface = !g_config.selectedInterface.empty();
    if (useSpecificInterface)
    {
        NetworkMonitor::NetworkStats selectedStats;
        if (g_pNetworkMonitor->GetInterfaceStats(g_config.selectedInterface, selectedStats))
        {
            stats = selectedStats;
        }
        else
        {
            stats = g_pNetworkMonitor->GetAggregatedStats();
        }
    }
    else
    {
        stats = g_pNetworkMonitor->GetAggregatedStats();
    }

    // ---------------------------------------------------------------------
    // History logging: record per-interval usage into SQLite (if available)
    // ---------------------------------------------------------------------
    unsigned long long totalDown = static_cast<unsigned long long>(stats.bytesReceived);
    unsigned long long totalUp   = static_cast<unsigned long long>(stats.bytesSent);

    if (!g_prevTotalsValid)
    {
        g_prevTotalBytesDown = totalDown;
        g_prevTotalBytesUp = totalUp;
        g_prevTotalsValid = true;
    }
    else
    {
        const unsigned long long MAX_VAL = (std::numeric_limits<unsigned long long>::max)();

        unsigned long long deltaDown = 0;
        unsigned long long deltaUp = 0;

        if (totalDown >= g_prevTotalBytesDown)
        {
            deltaDown = totalDown - g_prevTotalBytesDown;
        }
        else
        {
            // Counter wrapped
            deltaDown = (MAX_VAL - g_prevTotalBytesDown) + totalDown + 1;
        }

        if (totalUp >= g_prevTotalBytesUp)
        {
            deltaUp = totalUp - g_prevTotalBytesUp;
        }
        else
        {
            deltaUp = (MAX_VAL - g_prevTotalBytesUp) + totalUp + 1;
        }

        if (deltaDown > 0 || deltaUp > 0)
        {
            NetworkMonitor::HistoryLogger::Instance().AppendSample(
                stats.interfaceName.empty() ? std::wstring(L"All Interfaces") : stats.interfaceName,
                deltaDown,
                deltaUp
            );
        }

        g_prevTotalBytesDown = totalDown;
        g_prevTotalBytesUp = totalUp;
    }

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
            ShowSettingsDialog(g_hwnd);
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
        g_pTrayIcon->ShowContextMenu();
    }
}

void ShowSettingsDialog(HWND parent)
{
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG), parent, SettingsDialogProc, 0);
}

INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        PopulateSettingsDialog(hDlg);
        CenterDialogOnScreen(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (ApplySettingsFromDialog(hDlg))
            {
                EndDialog(hDlg, IDOK);
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

void PopulateSettingsDialog(HWND hDlg)
{
    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    struct IntervalOption
    {
        const wchar_t* label;
        UINT interval;
    };

    const IntervalOption intervals[] = {
        {L"Fast (1s)", UPDATE_INTERVAL_FAST},
        {L"Normal (2s)", UPDATE_INTERVAL_NORMAL},
        {L"Slow (5s)", UPDATE_INTERVAL_SLOW},
    };

    for (const auto& option : intervals)
    {
        int index = static_cast<int>(SendMessageW(hInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.label)));
        SendMessageW(hInterval, CB_SETITEMDATA, index, option.interval);
        if (g_config.updateInterval == option.interval)
        {
            SendMessageW(hInterval, CB_SETCURSEL, index, 0);
        }
    }

    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    struct UnitOption { const wchar_t* label; NetworkMonitor::SpeedUnit unit; };
    const UnitOption units[] = {
        {L"Bytes per second", NetworkMonitor::SpeedUnit::BytesPerSecond},
        {L"Kilobytes per second", NetworkMonitor::SpeedUnit::KiloBytesPerSecond},
        {L"Megabytes per second", NetworkMonitor::SpeedUnit::MegaBytesPerSecond},
        {L"Megabits per second", NetworkMonitor::SpeedUnit::MegaBitsPerSecond},
    };

    for (const auto& option : units)
    {
        int index = static_cast<int>(SendMessageW(hUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.label)));
        SendMessageW(hUnit, CB_SETITEMDATA, index, static_cast<WPARAM>(option.unit));
        if (g_config.displayUnit == option.unit)
        {
            SendMessageW(hUnit, CB_SETCURSEL, index, 0);
        }
    }

    PopulateInterfaceCombo(hDlg);

    CheckDlgButton(hDlg, IDC_AUTOSTART_CHECK, g_config.autoStart ? BST_CHECKED : BST_UNCHECKED);
}

bool ApplySettingsFromDialog(HWND hDlg)
{
    bool needsTimerUpdate = false;
    UINT newInterval = g_config.updateInterval;
    auto newUnit = g_config.displayUnit;
    bool newAutoStart = g_config.autoStart;
    std::wstring newInterface = g_config.selectedInterface;

    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    int sel = static_cast<int>(SendMessageW(hInterval, CB_GETCURSEL, 0, 0));
    if (sel != CB_ERR)
    {
        newInterval = static_cast<UINT>(SendMessageW(hInterval, CB_GETITEMDATA, sel, 0));
    }

    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    sel = static_cast<int>(SendMessageW(hUnit, CB_GETCURSEL, 0, 0));
    if (sel != CB_ERR)
    {
        newUnit = static_cast<NetworkMonitor::SpeedUnit>(SendMessageW(hUnit, CB_GETITEMDATA, sel, 0));
    }

    newAutoStart = (IsDlgButtonChecked(hDlg, IDC_AUTOSTART_CHECK) == BST_CHECKED);

    HWND hInterface = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
    sel = static_cast<int>(SendMessageW(hInterface, CB_GETCURSEL, 0, 0));
    if (sel != CB_ERR)
    {
        wchar_t buffer[256] = {0};
        SendMessageW(hInterface, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(buffer));
        if (sel == 0)
        {
            newInterface.clear();
        }
        else
        {
            newInterface = buffer;
        }
    }

    needsTimerUpdate = (newInterval != g_config.updateInterval);

    g_config.updateInterval = newInterval;
    g_config.displayUnit = newUnit;
    g_config.autoStart = newAutoStart;
    g_config.selectedInterface = newInterface;

    g_pConfigManager->SaveConfig(g_config);

    if (needsTimerUpdate)
    {
        KillTimer(g_hwnd, TIMER_UPDATE_NETWORK);
        SetTimer(g_hwnd, TIMER_UPDATE_NETWORK, g_config.updateInterval, nullptr);
    }

    // Force immediate refresh so UI reflects new settings
    OnTimer();

    return true;
}

void PopulateInterfaceCombo(HWND hDlg)
{
    HWND hInterface = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
    SendMessageW(hInterface, CB_RESETCONTENT, 0, 0);

    int index = static_cast<int>(SendMessageW(hInterface, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All Interfaces")));
    if (g_config.selectedInterface.empty())
    {
        SendMessageW(hInterface, CB_SETCURSEL, index, 0);
    }

    if (g_pNetworkMonitor)
    {
        std::vector<NetworkMonitor::NetworkStats> stats = g_pNetworkMonitor->GetAllStats();
        for (const auto& item : stats)
        {
            int addedIndex = static_cast<int>(SendMessageW(hInterface, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.interfaceName.c_str())));
            if (item.interfaceName == g_config.selectedInterface)
            {
                SendMessageW(hInterface, CB_SETCURSEL, addedIndex, 0);
            }
        }
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
