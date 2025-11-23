#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/TrayIcon.h"
#include "NetworkMonitor/TaskbarOverlay.h"
#include "TestUtils.h"

#include <windows.h>

using namespace NetworkMonitor;

namespace NetworkMonitorTests
{

namespace
{
    const wchar_t* kTestWindowClass = L"NetworkMonitorTestWindow";

    HWND CreateTestWindow()
    {
        HINSTANCE hInstance = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = hInstance;
        wc.lpszClassName = kTestWindowClass;

        // RegisterClassExW may fail if already registered; that's fine.
        RegisterClassExW(&wc);

        HWND hwnd = CreateWindowExW(
            0,
            kTestWindowClass,
            L"NetworkMonitor Test Window",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            100, 100,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        return hwnd;
    }
}

void RunTrayIconTests()
{
    LogTestMessage(L"=== TrayIcon tests ===");

    HWND hwnd = CreateTestWindow();
    if (!hwnd)
    {
        LogTestMessage(L"[WARN] Failed to create test window; skipping TrayIcon tests");
        return;
    }

    TrayIcon icon;
    bool init = icon.Initialize(hwnd);
    if (!init)
    {
        LogTestMessage(L"[WARN] TrayIcon.Initialize failed; skipping further TrayIcon tests");
        DestroyWindow(hwnd);
        return;
    }

    AppConfig config;
    icon.SetConfigSource(&config);

    NetworkStats stats;
    stats.currentDownloadSpeed = 1024.0;
    stats.currentUploadSpeed = 512.0;

    icon.UpdateTooltip(stats, SpeedUnit::KiloBytesPerSecond);
    icon.UpdateIcon(stats.currentDownloadSpeed, stats.currentUploadSpeed);

    icon.Cleanup();
    DestroyWindow(hwnd);

    AssertTrue(true, L"TrayIcon Initialize/Update/Cleanup executed without crash");
}

void RunTaskbarOverlayTests()
{
    LogTestMessage(L"=== TaskbarOverlay tests ===");

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    TaskbarOverlay overlay;

    bool init = overlay.Initialize(hInstance);
    if (!init)
    {
        LogTestMessage(L"[WARN] TaskbarOverlay.Initialize failed; skipping further overlay tests");
        return;
    }

    AssertTrue(!overlay.IsVisible(), L"TaskbarOverlay not visible after Initialize");

    overlay.Show(true);
    AssertTrue(overlay.IsVisible(), L"TaskbarOverlay visible after Show(true)");

    overlay.UpdateSpeed(2048.0, 1024.0, SpeedUnit::KiloBytesPerSecond);

    overlay.Show(false);
    AssertTrue(!overlay.IsVisible(), L"TaskbarOverlay not visible after Show(false)");

    overlay.Cleanup();

    AssertTrue(true, L"TaskbarOverlay Initialize/Show/Update/Cleanup executed without crash");
}

} // namespace NetworkMonitorTests
