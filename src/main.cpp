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
#include <commctrl.h>
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
void ShowDashboardDialog(HWND parent);
INT_PTR CALLBACK DashboardDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowHistoryManageDialog(HWND parent);
INT_PTR CALLBACK HistoryManageDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

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
        std::wstring msg = NetworkMonitor::LoadStringResource(IDS_ERROR_ALREADY_RUNNING);
        std::wstring title = NetworkMonitor::LoadStringResource(IDS_APP_TITLE);
        if (title.empty())
        {
            title = APP_NAME;
        }
        if (msg.empty())
        {
            msg = L"NetworkMonitor is already running!";
        }
        MessageBoxW(nullptr, msg.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
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
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

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
        NetworkMonitor::ShowErrorMessage(NetworkMonitor::LoadStringResource(IDS_ERR_REGISTER_WINDOW_CLASS));
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
        NetworkMonitor::ShowErrorMessage(NetworkMonitor::LoadStringResource(IDS_ERR_CREATE_WINDOW));
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
        NetworkMonitor::ShowErrorMessage(NetworkMonitor::LoadStringResource(IDS_ERR_INIT_TRAY_ICON));
        return false;
    }

    // Set menu callback
    g_pTrayIcon->SetMenuCallback(OnMenuCommand);
    g_pTrayIcon->SetConfigSource(&g_config);

    // THÊM MỚI: Create taskbar overlay
    g_pTaskbarOverlay = new NetworkMonitor::TaskbarOverlay();
    if (!g_pTaskbarOverlay->Initialize(hInstance))
    {
        NetworkMonitor::ShowErrorMessage(NetworkMonitor::LoadStringResource(IDS_ERR_INIT_TASKBAR_OVERLAY));
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
        NetworkMonitor::ShowErrorMessage(NetworkMonitor::LoadStringResource(IDS_ERR_START_NETWORK_MONITOR));
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

    if (g_config.enableLogging)
    {
        // -----------------------------------------------------------------
        // History logging: record per-interval usage into SQLite
        // NOTE: If counters ever go backwards (interface changed/reset),
        // we treat that as a reset and do NOT log a huge bogus delta.
        // -----------------------------------------------------------------
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
            unsigned long long deltaDown = 0;
            unsigned long long deltaUp = 0;

            if (totalDown >= g_prevTotalBytesDown)
            {
                deltaDown = totalDown - g_prevTotalBytesDown;
            }
            // else: counters decreased -> treat as reset, don't accumulate

            if (totalUp >= g_prevTotalBytesUp)
            {
                deltaUp = totalUp - g_prevTotalBytesUp;
            }
            // else: counters decreased -> treat as reset

            if (deltaDown > 0 || deltaUp > 0)
            {
                std::wstring ifaceName = stats.interfaceName;
                if (ifaceName.empty())
                {
                    ifaceName = NetworkMonitor::LoadStringResource(IDS_ALL_INTERFACES);
                    if (ifaceName.empty())
                    {
                        ifaceName = L"All Interfaces";
                    }
                }

                NetworkMonitor::HistoryLogger::Instance().AppendSample(
                    ifaceName,
                    deltaDown,
                    deltaUp
                );
            }

            g_prevTotalBytesDown = totalDown;
            g_prevTotalBytesUp = totalUp;
        }
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

        case IDM_DASHBOARD:
            ShowDashboardDialog(g_hwnd);
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

void ShowDashboardDialog(HWND parent)
{
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_DASHBOARD_DIALOG), parent, DashboardDialogProc, 0);
}

void ShowHistoryManageDialog(HWND parent)
{
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_HISTORY_MANAGE_DIALOG), parent, HistoryManageDialogProc, 0);
}

INT_PTR CALLBACK DashboardDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        CenterDialogOnScreen(hDlg);

        // Initialize list columns once
        HWND hList = GetDlgItem(hDlg, IDC_RECENT_LIST);
        if (hList)
        {
            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            LVCOLUMNW col = {};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;

            std::wstring timeHeader = NetworkMonitor::LoadStringResource(IDS_DASHBOARD_COL_TIME);
            if (timeHeader.empty())
            {
                timeHeader = L"Time";
            }
            col.pszText = const_cast<wchar_t*>(timeHeader.c_str());
            col.cx = 135;
            col.iSubItem = 0;
            col.fmt = LVCFMT_LEFT;
            ListView_InsertColumn(hList, 0, &col);

            std::wstring ifaceHeader = NetworkMonitor::LoadStringResource(IDS_DASHBOARD_COL_INTERFACE);
            if (ifaceHeader.empty())
            {
                ifaceHeader = L"Interface";
            }
            col.pszText = const_cast<wchar_t*>(ifaceHeader.c_str());
            col.cx = 110;
            col.iSubItem = 1;
            col.fmt = LVCFMT_LEFT;
            ListView_InsertColumn(hList, 1, &col);

            std::wstring downHeader = NetworkMonitor::LoadStringResource(IDS_DASHBOARD_COL_DOWN);
            if (downHeader.empty())
            {
                downHeader = L"Down";
            }
            col.pszText = const_cast<wchar_t*>(downHeader.c_str());
            col.cx = 85;
            col.iSubItem = 2;
            col.fmt = LVCFMT_RIGHT;
            ListView_InsertColumn(hList, 2, &col);

            std::wstring upHeader = NetworkMonitor::LoadStringResource(IDS_DASHBOARD_COL_UP);
            if (upHeader.empty())
            {
                upHeader = L"Up";
            }
            col.pszText = const_cast<wchar_t*>(upHeader.c_str());
            col.cx = 85;
            col.iSubItem = 3;
            col.fmt = LVCFMT_RIGHT;
            ListView_InsertColumn(hList, 3, &col);
        }

        // Fill data
        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DASHBOARD_REFRESH, 0), 0);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DASHBOARD_REFRESH:
        {
            unsigned long long todayDown = 0;
            unsigned long long todayUp = 0;
            unsigned long long monthDown = 0;
            unsigned long long monthUp = 0;

            NetworkMonitor::HistoryLogger& logger = NetworkMonitor::HistoryLogger::Instance();
            const std::wstring* ifaceFilter = nullptr;
            if (!g_config.selectedInterface.empty())
            {
                ifaceFilter = &g_config.selectedInterface;
            }

            logger.GetTotalsToday(todayDown, todayUp, ifaceFilter);
            logger.GetTotalsThisMonth(monthDown, monthUp, ifaceFilter);

            std::wstring todayDownStr = NetworkMonitor::FormatBytes(static_cast<ULONG64>(todayDown));
            std::wstring todayUpStr = NetworkMonitor::FormatBytes(static_cast<ULONG64>(todayUp));
            std::wstring monthDownStr = NetworkMonitor::FormatBytes(static_cast<ULONG64>(monthDown));
            std::wstring monthUpStr = NetworkMonitor::FormatBytes(static_cast<ULONG64>(monthUp));

            SetDlgItemTextW(hDlg, IDC_TODAY_DOWN, todayDownStr.c_str());
            SetDlgItemTextW(hDlg, IDC_TODAY_UP, todayUpStr.c_str());
            SetDlgItemTextW(hDlg, IDC_MONTH_DOWN, monthDownStr.c_str());
            SetDlgItemTextW(hDlg, IDC_MONTH_UP, monthUpStr.c_str());

            HWND hList = GetDlgItem(hDlg, IDC_RECENT_LIST);
            if (hList)
            {
                ListView_DeleteAllItems(hList);

                std::vector<NetworkMonitor::HistorySample> samples;
                logger.GetRecentSamples(100, samples, ifaceFilter, true /*onlyToday*/);

                int index = 0;
                for (const auto& sample : samples)
                {
                    wchar_t timeBuffer[64] = {};
                    std::tm localTime = {};
                    std::time_t ts = sample.timestamp;
                    if (localtime_s(&localTime, &ts) == 0)
                    {
                        wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &localTime);
                    }

                    LVITEMW item = {};
                    item.mask = LVIF_TEXT;
                    item.iItem = index;
                    item.iSubItem = 0;
                    item.pszText = timeBuffer;
                    int rowIndex = ListView_InsertItem(hList, &item);

                    std::wstring iface = sample.interfaceName;
                    if (iface.empty())
                    {
                        iface = NetworkMonitor::LoadStringResource(IDS_ALL_INTERFACES);
                        if (iface.empty())
                        {
                            iface = L"All Interfaces";
                        }
                    }
                    ListView_SetItemText(hList, rowIndex, 1, const_cast<wchar_t*>(iface.c_str()));

                    std::wstring downStr = NetworkMonitor::FormatBytes(static_cast<ULONG64>(sample.bytesDown));
                    std::wstring upStr = NetworkMonitor::FormatBytes(static_cast<ULONG64>(sample.bytesUp));

                    ListView_SetItemText(hList, rowIndex, 2, const_cast<wchar_t*>(downStr.c_str()));
                    ListView_SetItemText(hList, rowIndex, 3, const_cast<wchar_t*>(upStr.c_str()));

                    ++index;
                }

                // Auto-size columns to contents
                ListView_SetColumnWidth(hList, 0, LVSCW_AUTOSIZE_USEHEADER);
                ListView_SetColumnWidth(hList, 1, LVSCW_AUTOSIZE_USEHEADER);
                ListView_SetColumnWidth(hList, 2, LVSCW_AUTOSIZE_USEHEADER);
                ListView_SetColumnWidth(hList, 3, LVSCW_AUTOSIZE_USEHEADER);
            }
            return TRUE;
        }

        case IDC_HISTORY_MANAGE:
        {
            ShowHistoryManageDialog(hDlg);
            // After user modifies history, refresh dashboard view
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DASHBOARD_REFRESH, 0), 0);
            return TRUE;
        }

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
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
        UINT resourceId;
        UINT interval;
    };

    const IntervalOption intervals[] = {
        {IDS_INTERVAL_FAST, UPDATE_INTERVAL_FAST},
        {IDS_INTERVAL_NORMAL, UPDATE_INTERVAL_NORMAL},
        {IDS_INTERVAL_SLOW, UPDATE_INTERVAL_SLOW},
    };

    for (const auto& option : intervals)
    {
        std::wstring label = NetworkMonitor::LoadStringResource(option.resourceId);
        if (label.empty())
        {
            switch (option.interval)
            {
            case UPDATE_INTERVAL_FAST:
                label = L"Fast (1s)";
                break;
            case UPDATE_INTERVAL_NORMAL:
                label = L"Normal (2s)";
                break;
            case UPDATE_INTERVAL_SLOW:
                label = L"Slow (5s)";
                break;
            default:
                label = L"Unknown";
                break;
            }
        }

        int index = static_cast<int>(SendMessageW(hInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
        SendMessageW(hInterval, CB_SETITEMDATA, index, option.interval);
        if (g_config.updateInterval == option.interval)
        {
            SendMessageW(hInterval, CB_SETCURSEL, index, 0);
        }
    }

    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    struct UnitOption { UINT resourceId; NetworkMonitor::SpeedUnit unit; };
    const UnitOption units[] = {
        {IDS_UNIT_BYTES_PER_SECOND, NetworkMonitor::SpeedUnit::BytesPerSecond},
        {IDS_UNIT_KILOBYTES_PER_SECOND, NetworkMonitor::SpeedUnit::KiloBytesPerSecond},
        {IDS_UNIT_MEGABYTES_PER_SECOND, NetworkMonitor::SpeedUnit::MegaBytesPerSecond},
        {IDS_UNIT_MEGABITS_PER_SECOND, NetworkMonitor::SpeedUnit::MegaBitsPerSecond},
    };

    for (const auto& option : units)
    {
        std::wstring label = NetworkMonitor::LoadStringResource(option.resourceId);
        if (label.empty())
        {
            switch (option.unit)
            {
            case NetworkMonitor::SpeedUnit::BytesPerSecond:
                label = L"Bytes per second";
                break;
            case NetworkMonitor::SpeedUnit::KiloBytesPerSecond:
                label = L"Kilobytes per second";
                break;
            case NetworkMonitor::SpeedUnit::MegaBytesPerSecond:
                label = L"Megabytes per second";
                break;
            case NetworkMonitor::SpeedUnit::MegaBitsPerSecond:
                label = L"Megabits per second";
                break;
            default:
                label = L"Unknown";
                break;
            }
        }

        int index = static_cast<int>(SendMessageW(hUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
        SendMessageW(hUnit, CB_SETITEMDATA, index, static_cast<WPARAM>(option.unit));
        if (g_config.displayUnit == option.unit)
        {
            SendMessageW(hUnit, CB_SETCURSEL, index, 0);
        }
    }

    PopulateInterfaceCombo(hDlg);

    CheckDlgButton(hDlg, IDC_AUTOSTART_CHECK, g_config.autoStart ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_LOGGING_CHECK, g_config.enableLogging ? BST_CHECKED : BST_UNCHECKED);
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
    bool newEnableLogging = (IsDlgButtonChecked(hDlg, IDC_ENABLE_LOGGING_CHECK) == BST_CHECKED);

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
    g_config.enableLogging = newEnableLogging;
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

    std::wstring allLabel = NetworkMonitor::LoadStringResource(IDS_ALL_INTERFACES);
    if (allLabel.empty())
    {
        allLabel = L"All Interfaces";
    }

    int index = static_cast<int>(SendMessageW(hInterface, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(allLabel.c_str())));
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
    std::wstring title = NetworkMonitor::LoadStringResource(IDS_ABOUT_TITLE);
    if (title.empty())
    {
        title = L"About NetworkMonitor";
    }

    std::wstring versionLabel = NetworkMonitor::LoadStringResource(IDS_ABOUT_VERSION_LABEL);
    if (versionLabel.empty())
    {
        versionLabel = L"Version: ";
    }

    std::wstring body = NetworkMonitor::LoadStringResource(IDS_ABOUT_BODY);
    if (body.empty())
    {
        body = L"A lightweight network traffic monitor for Windows.\nDisplays real-time download/upload speeds in the system tray and taskbar.";
    }

    std::wstring message = APP_NAME;
    message += L"\n";
    message += versionLabel;
    message += APP_VERSION;
    message += L"\n\n";
    message += body;

    MessageBoxW(hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

// ============================================================================
// HISTORY MANAGEMENT DIALOG
// ============================================================================

INT_PTR CALLBACK HistoryManageDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        CenterDialogOnScreen(hDlg);

        std::wstring title = NetworkMonitor::LoadStringResource(IDS_HISTORY_MANAGE_TITLE);
        if (!title.empty())
        {
            SetWindowTextW(hDlg, title.c_str());
        }

        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_HISTORY_DELETE_ALL:
        case IDC_HISTORY_KEEP_30:
        case IDC_HISTORY_KEEP_90:
        {
            int days = 0;
            UINT confirmId = 0;

            if (LOWORD(wParam) == IDC_HISTORY_DELETE_ALL)
            {
                days = 0;
                confirmId = IDS_HISTORY_CONFIRM_DELETE_ALL;
            }
            else if (LOWORD(wParam) == IDC_HISTORY_KEEP_30)
            {
                days = 30;
                confirmId = IDS_HISTORY_CONFIRM_TRIM_30;
            }
            else if (LOWORD(wParam) == IDC_HISTORY_KEEP_90)
            {
                days = 90;
                confirmId = IDS_HISTORY_CONFIRM_TRIM_90;
            }

            std::wstring confirmText = NetworkMonitor::LoadStringResource(confirmId);
            if (confirmText.empty())
            {
                switch (LOWORD(wParam))
                {
                case IDC_HISTORY_DELETE_ALL:
                    confirmText = L"This will delete all logged history. Are you sure?";
                    break;
                case IDC_HISTORY_KEEP_30:
                    confirmText = L"Delete all records older than 30 days?";
                    break;
                case IDC_HISTORY_KEEP_90:
                    confirmText = L"Delete all records older than 90 days?";
                    break;
                }
            }

            std::wstring title = NetworkMonitor::LoadStringResource(IDS_HISTORY_MANAGE_TITLE);
            if (title.empty())
            {
                title = L"Manage History";
            }

            int res = MessageBoxW(hDlg, confirmText.c_str(), title.c_str(), MB_YESNO | MB_ICONQUESTION);
            if (res != IDYES)
            {
                return TRUE;
            }

            bool ok = false;
            auto& logger = NetworkMonitor::HistoryLogger::Instance();
            if (days <= 0)
            {
                ok = logger.DeleteAll();
            }
            else
            {
                ok = logger.TrimToRecentDays(days);
            }

            if (!ok)
            {
                std::wstring err = NetworkMonitor::LoadStringResource(IDS_HISTORY_ERROR_OPERATION);
                if (err.empty())
                {
                    err = L"Failed to modify history database.";
                }
                MessageBoxW(hDlg, err.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
            }

            return TRUE;
        }

        case IDCANCEL:
        case IDOK:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}
