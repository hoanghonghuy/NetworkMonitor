// ============================================================================
// File: SettingsDialog.cpp
// Description: Settings dialog implementation for NetworkMonitor
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/SettingsDialog.h"
#include "NetworkMonitor/ConfigManager.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/Utils.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>
#include <vector>

namespace NetworkMonitor
{

SettingsDialog::SettingsDialog()
    : m_hDialog(nullptr)
    , m_pConfigManager(nullptr)
    , m_pNetworkMonitor(nullptr)
    , m_isInitializing(false)
{
}

SettingsDialog::~SettingsDialog()
{
}

bool SettingsDialog::Show(HWND parentWindow, ConfigManager* configManager, NetworkMonitorClass* networkMonitor)
{
    if (!configManager)
    {
        return false;
    }

    m_pConfigManager = configManager;
    m_pNetworkMonitor = networkMonitor;
    m_isInitializing = true;

    // Load current config into working copy
    if (!m_pConfigManager->LoadConfig(m_configCopy))
    {
        m_configCopy = AppConfig(); // Use defaults if load fails
    }

    // Create modal dialog
    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    m_isInitializing = false;
    return (result == IDOK);
}

void SettingsDialog::SetSettingsChangedCallback(std::function<void()> callback)
{
    m_settingsChangedCallback = callback;
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SettingsDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<SettingsDialog*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDialog = hDlg;
    }
    else
    {
        pThis = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK SettingsDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Localize dialog caption and static controls using current resources
            std::wstring title = LoadStringResource(IDS_SETTINGS_TITLE);
            if (!title.empty())
            {
                SetWindowTextW(hDlg, title.c_str());
            }

            std::wstring generalText = LoadStringResource(IDS_SETTINGS_GROUP_GENERAL);
            if (!generalText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_GENERAL, generalText.c_str());
            }

            std::wstring updateText = LoadStringResource(IDS_SETTINGS_GROUP_UPDATE);
            if (!updateText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_UPDATE, updateText.c_str());
            }

            std::wstring networkText = LoadStringResource(IDS_SETTINGS_GROUP_NETWORK);
            if (!networkText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_NETWORK, networkText.c_str());
            }

            std::wstring langText = LoadStringResource(IDS_SETTINGS_LABEL_LANGUAGE);
            if (!langText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_LANGUAGE, langText.c_str());
            }

            std::wstring intervalText = LoadStringResource(IDS_SETTINGS_LABEL_INTERVAL);
            if (!intervalText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_INTERVAL, intervalText.c_str());
            }

            std::wstring monitorText = LoadStringResource(IDS_SETTINGS_LABEL_MONITOR);
            if (!monitorText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_MONITOR, monitorText.c_str());
            }

            std::wstring autostartText = LoadStringResource(IDS_SETTINGS_LABEL_AUTOSTART);
            if (!autostartText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_AUTOSTART_CHECK, autostartText.c_str());
            }

            std::wstring loggingText = LoadStringResource(IDS_SETTINGS_LABEL_LOGGING);
            if (!loggingText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_ENABLE_LOGGING_CHECK, loggingText.c_str());
            }

            std::wstring debugLogText = LoadStringResource(IDS_SETTINGS_LABEL_DEBUGLOGGING);
            if (!debugLogText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DEBUG_LOGGING_CHECK, debugLogText.c_str());
            }

            std::wstring darkThemeText = LoadStringResource(IDS_SETTINGS_LABEL_DARK_THEME);
            if (!darkThemeText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DARK_THEME_CHECK, darkThemeText.c_str());
            }

            std::wstring unitLabelText = LoadStringResource(IDS_SETTINGS_LABEL_SPEED_UNIT);
            if (!unitLabelText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_SPEED_UNIT, unitLabelText.c_str());
            }

            std::wstring trimLabelText = LoadStringResource(IDS_SETTINGS_LABEL_AUTOTRIM);
            if (!trimLabelText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_AUTOTRIM, trimLabelText.c_str());
            }

            std::wstring openLogText = LoadStringResource(IDS_SETTINGS_BUTTON_OPEN_LOG);
            if (!openLogText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_BUTTON_OPEN_LOG, openLogText.c_str());
            }

            // Populate dialog controls
            PopulateDialog(hDlg);
            CenterDialogOnScreen(hDlg);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_SETTINGS_BUTTON_OPEN_LOG:
                {
                    OpenLogFileInExplorer();
                    return TRUE;
                }

                case IDOK:
                {
                    if (ApplySettingsFromDialog(hDlg))
                    {
                        if (m_settingsChangedCallback)
                        {
                            m_settingsChangedCallback();
                        }
                        EndDialog(hDlg, IDOK);
                    }
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            if (m_configCopy.darkTheme)
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
    }

    return FALSE;
}

void SettingsDialog::PopulateDialog(HWND hDlg)
{
    // Populate language combo (mirrors main.cpp logic, using m_configCopy)
    HWND hLanguage = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
    if (hLanguage)
    {
        struct LanguageOption
        {
            AppLanguage language;
            UINT resourceId;
        };

        const LanguageOption langs[] = {
            {AppLanguage::SystemDefault, IDS_LANGUAGE_SYSTEM},
            {AppLanguage::English,       IDS_LANGUAGE_ENGLISH},
            {AppLanguage::Vietnamese,    IDS_LANGUAGE_VIETNAMESE},
        };

        int selectedIndex = -1;
        for (const auto& option : langs)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.language)
                {
                case AppLanguage::SystemDefault:
                    label = L"System (Windows default)";
                    break;
                case AppLanguage::English:
                    label = L"English";
                    break;
                case AppLanguage::Vietnamese:
                    label = L"Tiếng Việt";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hLanguage, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hLanguage, CB_SETITEMDATA, index, static_cast<WPARAM>(option.language));

            if (m_configCopy.language == option.language)
            {
                selectedIndex = index;
            }
        }

        if (selectedIndex >= 0)
        {
            SendMessageW(hLanguage, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hLanguage, CB_SETCURSEL, 0, 0);
        }
    }

    // Populate update interval combo
    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    if (hInterval)
    {
        struct IntervalOption
        {
            UINT resourceId;
            UINT interval;
        };

        const IntervalOption intervals[] = {
            {IDS_INTERVAL_FAST,   UPDATE_INTERVAL_FAST},
            {IDS_INTERVAL_NORMAL, UPDATE_INTERVAL_NORMAL},
            {IDS_INTERVAL_SLOW,   UPDATE_INTERVAL_SLOW},
        };

        for (const auto& option : intervals)
        {
            std::wstring label = LoadStringResource(option.resourceId);
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
            if (m_configCopy.updateInterval == option.interval)
            {
                SendMessageW(hInterval, CB_SETCURSEL, index, 0);
            }
        }
    }

    // Populate display unit combo
    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    if (hUnit)
    {
        struct UnitOption
        {
            UINT resourceId;
            SpeedUnit unit;
        };

        const UnitOption units[] = {
            {IDS_UNIT_BYTES_PER_SECOND,     SpeedUnit::BytesPerSecond},
            {IDS_UNIT_KILOBYTES_PER_SECOND, SpeedUnit::KiloBytesPerSecond},
            {IDS_UNIT_MEGABYTES_PER_SECOND, SpeedUnit::MegaBytesPerSecond},
            {IDS_UNIT_MEGABITS_PER_SECOND,  SpeedUnit::MegaBitsPerSecond},
        };

        for (const auto& option : units)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.unit)
                {
                case SpeedUnit::BytesPerSecond:
                    label = L"Bytes per second";
                    break;
                case SpeedUnit::KiloBytesPerSecond:
                    label = L"Kilobytes per second";
                    break;
                case SpeedUnit::MegaBytesPerSecond:
                    label = L"Megabytes per second";
                    break;
                case SpeedUnit::MegaBitsPerSecond:
                    label = L"Megabits per second";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hUnit, CB_SETITEMDATA, index, static_cast<WPARAM>(option.unit));
            if (m_configCopy.displayUnit == option.unit)
            {
                SendMessageW(hUnit, CB_SETCURSEL, index, 0);
            }
        }
    }

    // Populate interface combo
    PopulateInterfaceCombo(hDlg);

    // Set checkbox states (only controls that exist in current dialog)
    Button_SetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK), m_configCopy.autoStart ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK), m_configCopy.enableLogging ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK), m_configCopy.debugLogging ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_DARK_THEME_CHECK), m_configCopy.darkTheme ? BST_CHECKED : BST_UNCHECKED);

    // Populate history auto-trim combo
    HWND hTrim = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
    if (hTrim)
    {
        struct TrimOption
        {
            UINT days;
            UINT resourceId;
        };

        const TrimOption options[] = {
            {0,   IDS_HISTORY_AUTO_TRIM_NONE},
            {7,   IDS_HISTORY_AUTO_TRIM_7D},
            {30,  IDS_HISTORY_AUTO_TRIM_30D},
            {90,  IDS_HISTORY_AUTO_TRIM_90D},
            {365, IDS_HISTORY_AUTO_TRIM_365D},
        };

        int selectedIndex = -1;
        for (const auto& option : options)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                if (option.days == 0)
                {
                    label = L"Do not auto delete";
                }
                else
                {
                    wchar_t buffer[64] = {0};
                    swprintf_s(buffer, L"Keep last %u days", option.days);
                    label = buffer;
                }
            }

            int index = static_cast<int>(SendMessageW(hTrim, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hTrim, CB_SETITEMDATA, index, option.days);

            if (static_cast<UINT>(m_configCopy.historyAutoTrimDays) == option.days)
            {
                selectedIndex = index;
            }
        }

        if (selectedIndex >= 0)
        {
            SendMessageW(hTrim, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hTrim, CB_SETCURSEL, 0, 0);
        }
    }
}

bool SettingsDialog::ApplySettingsFromDialog(HWND hDlg)
{
    // Get update interval selection
    UINT newInterval = m_configCopy.updateInterval;
    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    if (hInterval)
    {
        int sel = static_cast<int>(SendMessageW(hInterval, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            newInterval = static_cast<UINT>(SendMessageW(hInterval, CB_GETITEMDATA, sel, 0));
        }
    }

    // Get display unit selection
    SpeedUnit newUnit = m_configCopy.displayUnit;
    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    if (hUnit)
    {
        int sel = static_cast<int>(SendMessageW(hUnit, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            newUnit = static_cast<SpeedUnit>(SendMessageW(hUnit, CB_GETITEMDATA, sel, 0));
        }
    }

    // Get checkbox states
    bool newAutoStart = (Button_GetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK)) == BST_CHECKED);
    bool newEnableLogging = (Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK)) == BST_CHECKED);
    bool newDebugLogging = (Button_GetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK)) == BST_CHECKED);
    bool newDarkTheme = (Button_GetCheck(GetDlgItem(hDlg, IDC_DARK_THEME_CHECK)) == BST_CHECKED);

    // Get interface selection
    std::wstring newInterface = m_configCopy.selectedInterface;
    HWND hInterface = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
    if (hInterface)
    {
        int sel = static_cast<int>(SendMessageW(hInterface, CB_GETCURSEL, 0, 0));
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
    }

    // Get history auto-trim selection
    int newTrimDays = m_configCopy.historyAutoTrimDays;
    HWND hTrim = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
    if (hTrim)
    {
        int selTrim = static_cast<int>(SendMessageW(hTrim, CB_GETCURSEL, 0, 0));
        if (selTrim != CB_ERR)
        {
            LRESULT days = SendMessageW(hTrim, CB_GETITEMDATA, selTrim, 0);
            if (days != CB_ERR)
            {
                newTrimDays = static_cast<int>(days);
            }
        }
    }

    // Get language selection
    AppLanguage newLanguage = m_configCopy.language;
    HWND hLanguage = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
    if (hLanguage)
    {
        int selLang = static_cast<int>(SendMessageW(hLanguage, CB_GETCURSEL, 0, 0));
        if (selLang != CB_ERR)
        {
            LRESULT langVal = SendMessageW(hLanguage, CB_GETITEMDATA, selLang, 0);
            if (langVal != CB_ERR)
            {
                newLanguage = static_cast<AppLanguage>(langVal);
            }
        }
    }

    // Update working copy
    m_configCopy.updateInterval = newInterval;
    m_configCopy.displayUnit = newUnit;
    m_configCopy.autoStart = newAutoStart;
    m_configCopy.enableLogging = newEnableLogging;
    m_configCopy.debugLogging = newDebugLogging;
    m_configCopy.darkTheme = newDarkTheme;
    m_configCopy.selectedInterface = newInterface;
    m_configCopy.historyAutoTrimDays = newTrimDays;
    m_configCopy.language = newLanguage;

    // Save to registry via ConfigManager (ignore errors for now, like main.cpp)
    if (m_pConfigManager)
    {
        m_pConfigManager->SaveConfig(m_configCopy);
    }

    return true;
}

void SettingsDialog::PopulateInterfaceCombo(HWND hDlg)
{
    HWND hInterface = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
    if (!hInterface)
    {
        return;
    }

    ComboBox_ResetContent(hInterface);

    // Add "All Interfaces" option
    std::wstring allLabel = LoadStringResource(IDS_ALL_INTERFACES);
    if (allLabel.empty())
    {
        allLabel = L"All Interfaces";
    }

    int indexAll = ComboBox_AddString(hInterface, allLabel.c_str());
    if (m_configCopy.selectedInterface.empty())
    {
        ComboBox_SetCurSel(hInterface, indexAll);
    }

    // TODO: When NetworkMonitorClass is available here, enumerate real interfaces.
    // For now, add the currently selected interface (if any) so the user sees it.
    if (m_pNetworkMonitor)
    {
        std::vector<NetworkStats> statsList = m_pNetworkMonitor->GetAllStats();
        int selectedIdx = -1;

        for (const auto& stats : statsList)
        {
            if (stats.interfaceName.empty())
            {
                continue;
            }

            int idx = ComboBox_AddString(hInterface, stats.interfaceName.c_str());
            if (stats.interfaceName == m_configCopy.selectedInterface)
            {
                selectedIdx = idx;
            }
        }

        if (selectedIdx >= 0)
        {
            ComboBox_SetCurSel(hInterface, selectedIdx);
        }
        else if (!m_configCopy.selectedInterface.empty())
        {
            int idx = ComboBox_AddString(hInterface, m_configCopy.selectedInterface.c_str());
            ComboBox_SetCurSel(hInterface, idx);
        }
    }
    else if (!m_configCopy.selectedInterface.empty())
    {
        int idx = ComboBox_AddString(hInterface, m_configCopy.selectedInterface.c_str());
        ComboBox_SetCurSel(hInterface, idx);
    }
}

void SettingsDialog::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
}

} // namespace NetworkMonitor
