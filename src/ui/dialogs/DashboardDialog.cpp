// ============================================================================
// File: DashboardDialog.cpp
// Description: Dashboard dialog implementation for NetworkMonitor
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/DashboardDialog.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/HistoryLogger.h"
#include "NetworkMonitor/HistoryDialog.h"
#include "NetworkMonitor/Utils.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <sstream>
#include <ctime>

// Fix Windows macro conflicts
#undef max
#undef min

namespace NetworkMonitor
{

DashboardDialog::DashboardDialog()
    : m_hDialog(nullptr)
    , m_pNetworkMonitor(nullptr)
    , m_pConfig(nullptr)
{
}

DashboardDialog::~DashboardDialog()
{
}

bool DashboardDialog::Show(HWND parentWindow, NetworkMonitorClass* networkMonitor, const AppConfig* config)
{
    if (!networkMonitor || !config)
    {
        return false;
    }

    m_pNetworkMonitor = networkMonitor;
    m_pConfig = config;

    // Create modal dialog
    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_DASHBOARD_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return (result == IDOK);
}

INT_PTR CALLBACK DashboardDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DashboardDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<DashboardDialog*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDialog = hDlg;
    }
    else
    {
        pThis = reinterpret_cast<DashboardDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK DashboardDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Mirror DashboardDialogProc initialization from main.cpp
            CenterDialogOnScreen(hDlg);

            std::wstring dashTitle = LoadStringResource(IDS_DASHBOARD_TITLE);
            if (!dashTitle.empty())
            {
                SetWindowTextW(hDlg, dashTitle.c_str());
            }

            std::wstring todayLabel = LoadStringResource(IDS_DASHBOARD_LABEL_TODAY);
            if (!todayLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_TODAY, todayLabel.c_str());
            }

            std::wstring monthLabel = LoadStringResource(IDS_DASHBOARD_LABEL_THIS_MONTH);
            if (!monthLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_MONTH, monthLabel.c_str());
            }

            std::wstring dlLabel = LoadStringResource(IDS_DASHBOARD_LABEL_DOWNLOAD);
            if (!dlLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_DOWNLOAD_T, dlLabel.c_str());
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_DOWNLOAD_M, dlLabel.c_str());
            }

            std::wstring ulLabel = LoadStringResource(IDS_DASHBOARD_LABEL_UPLOAD);
            if (!ulLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_UPLOAD_T, ulLabel.c_str());
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_UPLOAD_M, ulLabel.c_str());
            }

            // Initialize list columns once
            HWND hList = GetDlgItem(hDlg, IDC_RECENT_LIST);
            if (hList)
            {
                ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

                LVCOLUMNW col = {};
                col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;

                std::wstring timeHeader = LoadStringResource(IDS_DASHBOARD_COL_TIME);
                if (timeHeader.empty())
                {
                    timeHeader = L"Time";
                }
                col.pszText = const_cast<wchar_t*>(timeHeader.c_str());
                col.cx = 135;
                col.iSubItem = 0;
                col.fmt = LVCFMT_LEFT;
                ListView_InsertColumn(hList, 0, &col);

                std::wstring ifaceHeader = LoadStringResource(IDS_DASHBOARD_COL_INTERFACE);
                if (ifaceHeader.empty())
                {
                    ifaceHeader = L"Interface";
                }
                col.pszText = const_cast<wchar_t*>(ifaceHeader.c_str());
                col.cx = 110;
                col.iSubItem = 1;
                col.fmt = LVCFMT_LEFT;
                ListView_InsertColumn(hList, 1, &col);

                std::wstring downHeader = LoadStringResource(IDS_DASHBOARD_COL_DOWN);
                if (downHeader.empty())
                {
                    downHeader = L"Down";
                }
                col.pszText = const_cast<wchar_t*>(downHeader.c_str());
                col.cx = 85;
                col.iSubItem = 2;
                col.fmt = LVCFMT_RIGHT;
                ListView_InsertColumn(hList, 2, &col);

                std::wstring upHeader = LoadStringResource(IDS_DASHBOARD_COL_UP);
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
            PostMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DASHBOARD_REFRESH, 0), 0);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_DASHBOARD_REFRESH:
                {
                    UpdateDashboardData(hDlg);

                    HWND hChart = GetDlgItem(hDlg, IDC_DASHBOARD_CHART);
                    if (hChart)
                    {
                        InvalidateRect(hChart, nullptr, TRUE);
                    }
                    return TRUE;
                }

                case IDC_HISTORY_MANAGE:
                {
                    // Use HistoryDialog class instead of legacy main.cpp dialog
                    HistoryDialog dlg;
                    dlg.Show(hDlg);
                    // After user modifies history, refresh dashboard view
                    PostMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DASHBOARD_REFRESH, 0), 0);
                    return TRUE;
                }

                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }
            }
            break;
        }

        case WM_DRAWITEM:
        {
            if (wParam == IDC_DASHBOARD_CHART)
            {
                DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (pDrawItem->CtlType == ODT_STATIC)
                {
                    HDC hdc = pDrawItem->hDC;
                    RECT rc = pDrawItem->rcItem;
                    DrawDashboardChart(hdc, rc);
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

void DashboardDialog::UpdateDashboardData(HWND hDlg)
{
    unsigned long long todayDown = 0;
    unsigned long long todayUp = 0;
    unsigned long long monthDown = 0;
    unsigned long long monthUp = 0;

    HistoryLogger& logger = HistoryLogger::Instance();
    const std::wstring* ifaceFilter = nullptr;

    if (m_pConfig && !m_pConfig->selectedInterface.empty())
    {
        ifaceFilter = &m_pConfig->selectedInterface;
    }

    logger.GetTotalsToday(todayDown, todayUp, ifaceFilter);
    logger.GetTotalsThisMonth(monthDown, monthUp, ifaceFilter);

    std::wstring todayDownStr = FormatBytes(static_cast<ULONG64>(todayDown));
    std::wstring todayUpStr = FormatBytes(static_cast<ULONG64>(todayUp));
    std::wstring monthDownStr = FormatBytes(static_cast<ULONG64>(monthDown));
    std::wstring monthUpStr = FormatBytes(static_cast<ULONG64>(monthUp));

    SetDlgItemTextW(hDlg, IDC_TODAY_DOWN, todayDownStr.c_str());
    SetDlgItemTextW(hDlg, IDC_TODAY_UP, todayUpStr.c_str());
    SetDlgItemTextW(hDlg, IDC_MONTH_DOWN, monthDownStr.c_str());
    SetDlgItemTextW(hDlg, IDC_MONTH_UP, monthUpStr.c_str());

    // Populate recent samples list
    HWND hList = GetDlgItem(hDlg, IDC_RECENT_LIST);
    if (hList)
    {
        ListView_DeleteAllItems(hList);

        std::vector<HistorySample> samples;
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
                iface = LoadStringResource(IDS_ALL_INTERFACES);
                if (iface.empty())
                {
                    iface = L"All Interfaces";
                }
            }
            ListView_SetItemText(hList, rowIndex, 1, const_cast<wchar_t*>(iface.c_str()));

            std::wstring downStr = FormatBytes(static_cast<ULONG64>(sample.bytesDown));
            std::wstring upStr = FormatBytes(static_cast<ULONG64>(sample.bytesUp));

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
}

void DashboardDialog::DrawDashboardChart(HDC hdc, const RECT& rc)
{
    if (!hdc)
    {
        return;
    }

    HBRUSH backBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(hdc, &rc, backBrush);
    DeleteObject(backBrush);

    HistoryLogger& logger = HistoryLogger::Instance();
    const std::wstring* ifaceFilter = nullptr;
    if (m_pConfig && !m_pConfig->selectedInterface.empty())
    {
        ifaceFilter = &m_pConfig->selectedInterface;
    }

    std::vector<HistorySample> samples;
    logger.GetRecentSamples(100, samples, ifaceFilter, true);

    if (samples.empty())
    {
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        return;
    }

    unsigned long long maxValue = 0;
    for (const auto& s : samples)
    {
        maxValue = (std::max)(maxValue, s.bytesDown);
        maxValue = (std::max)(maxValue, s.bytesUp);
    }

    if (maxValue == 0)
    {
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        return;
    }

    RECT inner = rc;
    inner.left += 4;
    inner.right -= 4;
    inner.top += 4;
    inner.bottom -= 4;

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, borderPen));
    Rectangle(hdc, inner.left, inner.top, inner.right, inner.bottom);

    int width = inner.right - inner.left;
    int height = inner.bottom - inner.top;
    size_t count = samples.size();

    if (count == 0 || width <= 0 || height <= 0)
    {
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
        return;
    }

    auto getX = [&](size_t index) -> int {
        if (count <= 1)
        {
            return inner.left + width / 2;
        }
        return inner.left + static_cast<int>((static_cast<double>(index) * static_cast<double>(width - 1)) / static_cast<double>(count - 1));
    };

    auto getY = [&](unsigned long long value) -> int {
        double ratio = static_cast<double>(value) / static_cast<double>(maxValue);
        if (ratio < 0.0)
        {
            ratio = 0.0;
        }
        if (ratio > 1.0)
        {
            ratio = 1.0;
        }
        return inner.bottom - static_cast<int>(ratio * static_cast<double>(height));
    };

    std::vector<POINT> downPoints;
    std::vector<POINT> upPoints;
    downPoints.reserve(count);
    upPoints.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        const auto& s = samples[count - 1 - i];
        POINT pd;
        pd.x = getX(i);
        pd.y = getY(s.bytesDown);
        downPoints.push_back(pd);

        POINT pu;
        pu.x = pd.x;
        pu.y = getY(s.bytesUp);
        upPoints.push_back(pu);
    }

    HPEN downPen = CreatePen(PS_SOLID, 1, RGB(0, 128, 0));
    HPEN upPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 200));

    SelectObject(hdc, downPen);
    if (!downPoints.empty())
    {
        MoveToEx(hdc, downPoints[0].x, downPoints[0].y, nullptr);
        for (size_t i = 1; i < downPoints.size(); ++i)
        {
            LineTo(hdc, downPoints[i].x, downPoints[i].y);
        }
    }

    SelectObject(hdc, upPen);
    if (!upPoints.empty())
    {
        MoveToEx(hdc, upPoints[0].x, upPoints[0].y, nullptr);
        for (size_t i = 1; i < upPoints.size(); ++i)
        {
            LineTo(hdc, upPoints[i].x, upPoints[i].y);
        }
    }

    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    int legendX = inner.left + 8;
    int legendY = inner.top + 8;

    SelectObject(hdc, downPen);
    MoveToEx(hdc, legendX, legendY, nullptr);
    LineTo(hdc, legendX + 20, legendY);

    std::wstring downLabel = LoadStringResource(IDS_DASHBOARD_COL_DOWN);
    if (downLabel.empty())
    {
        downLabel = L"Down";
    }

    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    TextOutW(hdc, legendX + 24, legendY - 6, downLabel.c_str(), static_cast<int>(downLabel.size()));

    legendY += 14;

    SelectObject(hdc, upPen);
    MoveToEx(hdc, legendX, legendY, nullptr);
    LineTo(hdc, legendX + 20, legendY);

    std::wstring upLabel = LoadStringResource(IDS_DASHBOARD_COL_UP);
    if (upLabel.empty())
    {
        upLabel = L"Up";
    }

    TextOutW(hdc, legendX + 24, legendY - 6, upLabel.c_str(), static_cast<int>(upLabel.size()));

    SetBkMode(hdc, oldBkMode);

    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
    DeleteObject(downPen);
    DeleteObject(upPen);
}

void DashboardDialog::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
}

} // namespace NetworkMonitor
