// ============================================================================
// File: HistoryDialog.cpp
// Description: History management dialog implementation for NetworkMonitor
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/HistoryDialog.h"
#include "NetworkMonitor/HistoryLogger.h"
#include "NetworkMonitor/Utils.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>

namespace NetworkMonitor
{

HistoryDialog::HistoryDialog()
    : m_hDialog(nullptr)
    , m_pConfig(nullptr)
{
}

HistoryDialog::~HistoryDialog()
{
}

bool HistoryDialog::Show(HWND parentWindow, const AppConfig* config)
{
    m_pConfig = config;
    // Create modal dialog
    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_HISTORY_MANAGE_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return (result == IDOK);
}

INT_PTR CALLBACK HistoryDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HistoryDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<HistoryDialog*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDialog = hDlg;
    }
    else
    {
        pThis = reinterpret_cast<HistoryDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK HistoryDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Mirror HistoryManageDialogProc initialization from main.cpp
            CenterDialogOnScreen(hDlg);

            std::wstring title = LoadStringResource(IDS_HISTORY_DIALOG_TITLE);
            if (!title.empty())
            {
                SetWindowTextW(hDlg, title.c_str());
            }

            std::wstring opsLabel = LoadStringResource(IDS_HISTORY_LABEL_OPERATIONS);
            if (!opsLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_LABEL_OPERATIONS, opsLabel.c_str());
            }

            std::wstring btnDelete = LoadStringResource(IDS_HISTORY_BUTTON_DELETE_ALL);
            if (!btnDelete.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_DELETE_ALL, btnDelete.c_str());
            }

            std::wstring btn30 = LoadStringResource(IDS_HISTORY_BUTTON_KEEP_30);
            if (!btn30.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_KEEP_30, btn30.c_str());
            }

            std::wstring btn90 = LoadStringResource(IDS_HISTORY_BUTTON_KEEP_90);
            if (!btn90.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_KEEP_90, btn90.c_str());
            }

            return TRUE;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            if (m_pConfig && m_pConfig->darkTheme)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                static HBRUSH s_darkBrush = nullptr;
                if (!s_darkBrush)
                {
                    s_darkBrush = CreateSolidBrush(RGB(32, 32, 32));
                }

                SetTextColor(hdc, RGB(230, 230, 230));
                SetBkMode(hdc, TRANSPARENT);

                return reinterpret_cast<INT_PTR>(s_darkBrush);
            }
            break;
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

                    std::wstring confirmText = LoadStringResource(confirmId);
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

                    std::wstring title = LoadStringResource(IDS_HISTORY_MANAGE_TITLE);
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
                    HistoryLogger& logger = HistoryLogger::Instance();
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
                        std::wstring err = LoadStringResource(IDS_HISTORY_ERROR_OPERATION);
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

void HistoryDialog::UpdateHistoryInfo(HWND hDlg)
{
    UNREFERENCED_PARAMETER(hDlg);
    // No additional info section in current dialog resource.
}

void HistoryDialog::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
}

} // namespace NetworkMonitor
