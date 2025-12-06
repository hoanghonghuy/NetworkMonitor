// ============================================================================
// File: SettingsDialog.cpp
// Description: Settings dialog implementation for NetworkMonitor
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/SettingsDialog.h"
#include "NetworkMonitor/ConfigManager.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/Utils.h"
#include "NetworkMonitor/ThemeHelper.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
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

            // Apply dark title bar if enabled
            ThemeHelper::ApplyDarkTitleBar(hDlg, m_configCopy.darkTheme);

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
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_THEME, darkThemeText.c_str());
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

            // In dark theme, make bottom buttons owner-drawn so we can paint
            // dark backgrounds consistently.
            if (m_configCopy.darkTheme)
            {
                auto makeOwnerDraw = [](HWND hButton)
                {
                    if (!hButton) return;
                    LONG_PTR style = GetWindowLongPtrW(hButton, GWL_STYLE);
                    if ((style & BS_OWNERDRAW) == 0)
                    {
                        style &= ~BS_TYPEMASK;  // clear existing button type (e.g., BS_DEFPUSHBUTTON)
                        style |= BS_OWNERDRAW;
                        SetWindowLongPtrW(hButton, GWL_STYLE, style);

                        // Force immediate redraw using owner-draw logic so
                        // the initial frame/background is also dark.
                        InvalidateRect(hButton, nullptr, TRUE);
                        UpdateWindow(hButton);
                    }
                };

                makeOwnerDraw(GetDlgItem(hDlg, IDC_SETTINGS_BUTTON_OPEN_LOG));
                makeOwnerDraw(GetDlgItem(hDlg, IDOK));
                makeOwnerDraw(GetDlgItem(hDlg, IDCANCEL));

                // Clear default button to prevent the system from drawing an
                // initial white default highlight before owner-draw kicks in.
                SendMessageW(hDlg, DM_SETDEFID, 0, 0);
            }
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
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
        {
            if (m_configCopy.darkTheme)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                static HBRUSH s_darkBrush = nullptr;
                if (!s_darkBrush)
                {
                    s_darkBrush = CreateSolidBrush(RGB(32, 32, 32));
                }

                HWND hwndCtl = reinterpret_cast<HWND>(lParam);
                int ctrlId = GetDlgCtrlID(hwndCtl);

                bool isComboArea =
                    (ctrlId == IDC_LANGUAGE_COMBO) ||
                    (ctrlId == IDC_UPDATE_INTERVAL_COMBO) ||
                    (ctrlId == IDC_DISPLAY_UNIT_COMBO) ||
                    (ctrlId == IDC_INTERFACE_COMBO) ||
                    (ctrlId == IDC_HISTORY_AUTO_TRIM_COMBO) ||
                    (ctrlId == IDC_THEME_MODE_COMBO);

                if (message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLOREDIT || isComboArea)
                {
                    // For combobox edit/dropdown areas: fill opaque dark background
                    SetTextColor(hdc, RGB(230, 230, 230));
                    SetBkColor(hdc, RGB(32, 32, 32));
                    SetBkMode(hdc, OPAQUE);
                }
                else
                {
                    // For labels, group boxes, buttons: transparent over dark dialog
                    SetTextColor(hdc, RGB(230, 230, 230));
                    SetBkMode(hdc, TRANSPARENT);
                }

                return reinterpret_cast<INT_PTR>(s_darkBrush);
            }
            break;
        }
        case WM_DRAWITEM:
        {
            if (m_configCopy.darkTheme)
            {
                DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (pDrawItem->CtlType == ODT_BUTTON)
                {
                    UINT id = pDrawItem->CtlID;
                    if (id == IDC_SETTINGS_BUTTON_OPEN_LOG || id == IDOK || id == IDCANCEL)
                    {
                        HDC hdc = pDrawItem->hDC;
                        RECT rc = pDrawItem->rcItem;

                        bool pressed = (pDrawItem->itemState & ODS_SELECTED) != 0;
                        bool focused = (pDrawItem->itemState & ODS_FOCUS) != 0;
                        bool disabled = (pDrawItem->itemState & ODS_DISABLED) != 0;

                        COLORREF backColor = pressed ? RGB(50, 50, 50) : RGB(40, 40, 40);
                        COLORREF borderColor = RGB(90, 90, 90);
                        COLORREF textColor = disabled ? RGB(160, 160, 160) : RGB(230, 230, 230);

                        HBRUSH hBrush = CreateSolidBrush(backColor);
                        FillRect(hdc, &rc, hBrush);
                        DeleteObject(hBrush);

                        // Draw only the frame so we don't overwrite the
                        // dark background we just filled.
                        HBRUSH hBorder = CreateSolidBrush(borderColor);
                        FrameRect(hdc, &rc, hBorder);
                        DeleteObject(hBorder);

                        wchar_t text[128] = {0};
                        GetWindowTextW(pDrawItem->hwndItem, text, static_cast<int>(std::size(text)));

                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, textColor);

                        RECT textRc = rc;
                        InflateRect(&textRc, -4, -2);
                        DrawTextW(hdc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                        if (focused)
                        {
                            RECT focusRc = rc;
                            InflateRect(&focusRc, -3, -3);
                            DrawFocusRect(hdc, &focusRc);
                        }

                        return TRUE;
                    }
                }
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

    // Populate theme mode combo
    HWND hThemeMode = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
    if (hThemeMode)
    {
        struct ThemeOption
        {
            ThemeMode mode;
            UINT resourceId;
        };

        const ThemeOption themes[] = {
            {ThemeMode::SystemDefault, IDS_SETTINGS_THEME_SYSTEM},
            {ThemeMode::Light,         IDS_SETTINGS_THEME_LIGHT},
            {ThemeMode::Dark,          IDS_SETTINGS_THEME_DARK},
        };

        int selectedIndex = -1;
        for (const auto& option : themes)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.mode)
                {
                case ThemeMode::SystemDefault:
                    label = L"System (Windows default)";
                    break;
                case ThemeMode::Light:
                    label = L"Light";
                    break;
                case ThemeMode::Dark:
                    label = L"Dark";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hThemeMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hThemeMode, CB_SETITEMDATA, index, static_cast<WPARAM>(static_cast<int>(option.mode)));

            if (m_configCopy.themeMode == option.mode)
            {
                selectedIndex = index;
            }
        }

        if (selectedIndex >= 0)
        {
            SendMessageW(hThemeMode, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hThemeMode, CB_SETCURSEL, 0, 0);
        }
    }

    // Populate interface combo
    PopulateInterfaceCombo(hDlg);

    // Set checkbox states (only controls that exist in current dialog)
    Button_SetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK), m_configCopy.autoStart ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK), m_configCopy.enableLogging ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK), m_configCopy.debugLogging ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK), m_configCopy.enableConnectionNotification ? BST_CHECKED : BST_UNCHECKED);

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

    // Populate ping target edit
    HWND hPingTarget = GetDlgItem(hDlg, IDC_PING_TARGET_EDIT);
    if (hPingTarget)
    {
        SetWindowTextW(hPingTarget, m_configCopy.pingTarget.c_str());
    }

    // Populate ping interval combo
    HWND hPingInterval = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
    if (hPingInterval)
    {
        struct IntervalOption { UINT ms; const wchar_t* label; };
        const IntervalOption intervals[] = {
            {3000,  L"3s"},
            {5000,  L"5s"},
            {10000, L"10s"},
            {30000, L"30s"},
        };

        int selectedIndex = -1;
        for (const auto& option : intervals)
        {
            int index = static_cast<int>(SendMessageW(hPingInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.label)));
            SendMessageW(hPingInterval, CB_SETITEMDATA, index, option.ms);
            if (m_configCopy.pingIntervalMs == option.ms)
            {
                selectedIndex = index;
            }
        }
        if (selectedIndex >= 0)
        {
            SendMessageW(hPingInterval, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hPingInterval, CB_SETCURSEL, 1, 0); // Default to 5s
        }
    }

    // Populate hotkey combo
    HWND hHotkey = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
    if (hHotkey)
    {
        struct HotkeyOption { UINT modifier; UINT key; const wchar_t* label; };
        const HotkeyOption hotkeys[] = {
            {MOD_WIN | MOD_SHIFT, 'N', L"Win+Shift+N"},
            {MOD_WIN | MOD_SHIFT, 'M', L"Win+Shift+M"},
            {MOD_CONTROL | MOD_SHIFT, 'N', L"Ctrl+Shift+N"},
            {MOD_CONTROL | MOD_ALT, 'N', L"Ctrl+Alt+N"},
        };

        int selectedIndex = -1;
        for (const auto& option : hotkeys)
        {
            int index = static_cast<int>(SendMessageW(hHotkey, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.label)));
            // Store both modifier and key in item data (modifier in high word, key in low word)
            LPARAM data = MAKELPARAM(option.key, option.modifier);
            SendMessageW(hHotkey, CB_SETITEMDATA, index, data);
            if (m_configCopy.hotkeyModifier == option.modifier && m_configCopy.hotkeyKey == option.key)
            {
                selectedIndex = index;
            }
        }
        if (selectedIndex >= 0)
        {
            SendMessageW(hHotkey, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hHotkey, CB_SETCURSEL, 0, 0); // Default to Win+Shift+N
        }
    }

    // For dark theme, disable visual styles for comboboxes so our
    // WM_CTLCOLOR* handlers can control background/text colors.
    if (m_configCopy.darkTheme)
    {
        HWND hLangTheme   = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
        HWND hIntTheme    = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
        HWND hUnitTheme   = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
        HWND hIfaceTheme  = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
        HWND hTrimTheme   = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
        HWND hThemeModeCB = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
        HWND hPingIntTheme = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
        HWND hHotkeyTheme = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);

        if (hLangTheme)    SetWindowTheme(hLangTheme,   L"", L"");
        if (hIntTheme)     SetWindowTheme(hIntTheme,    L"", L"");
        if (hUnitTheme)    SetWindowTheme(hUnitTheme,   L"", L"");
        if (hIfaceTheme)   SetWindowTheme(hIfaceTheme,  L"", L"");
        if (hTrimTheme)    SetWindowTheme(hTrimTheme,   L"", L"");
        if (hThemeModeCB)  SetWindowTheme(hThemeModeCB, L"", L"");
        if (hPingIntTheme) SetWindowTheme(hPingIntTheme, L"", L"");
        if (hHotkeyTheme)  SetWindowTheme(hHotkeyTheme, L"", L"");
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
    bool newConnectionNotify = (Button_GetCheck(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK)) == BST_CHECKED);
    bool newDarkTheme = m_configCopy.darkTheme;

    // Keep ThemeMode roughly in sync with the dark theme checkbox so that
    // future logic based on ThemeMode can distinguish between explicit
    // Light/Dark overrides (while SystemDefault remains the migrated value
    // when the user has not changed the theme setting yet).
    ThemeMode newThemeMode = m_configCopy.themeMode;
    HWND hThemeModeDlg = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
    if (hThemeModeDlg)
    {
        int selTheme = static_cast<int>(SendMessageW(hThemeModeDlg, CB_GETCURSEL, 0, 0));
        if (selTheme != CB_ERR)
        {
            LRESULT data = SendMessageW(hThemeModeDlg, CB_GETITEMDATA, selTheme, 0);
            if (data != CB_ERR)
            {
                newThemeMode = static_cast<ThemeMode>(static_cast<int>(data));
            }
        }
    }

    AppConfig tempConfig = m_configCopy;
    tempConfig.themeMode = newThemeMode;
    newDarkTheme = IsDarkThemeEnabled(tempConfig);

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

    // Get ping target
    std::wstring newPingTarget = m_configCopy.pingTarget;
    HWND hPingTarget = GetDlgItem(hDlg, IDC_PING_TARGET_EDIT);
    if (hPingTarget)
    {
        wchar_t buffer[256] = {0};
        GetWindowTextW(hPingTarget, buffer, 256);
        newPingTarget = buffer;
        if (newPingTarget.empty())
        {
            newPingTarget = L"8.8.8.8";
        }
    }

    // Get ping interval
    UINT newPingInterval = m_configCopy.pingIntervalMs;
    HWND hPingInterval = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
    if (hPingInterval)
    {
        int sel = static_cast<int>(SendMessageW(hPingInterval, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hPingInterval, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                newPingInterval = static_cast<UINT>(data);
            }
        }
    }

    // Get hotkey
    UINT newHotkeyModifier = m_configCopy.hotkeyModifier;
    UINT newHotkeyKey = m_configCopy.hotkeyKey;
    HWND hHotkeyCombo = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
    if (hHotkeyCombo)
    {
        int sel = static_cast<int>(SendMessageW(hHotkeyCombo, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hHotkeyCombo, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                newHotkeyKey = LOWORD(data);
                newHotkeyModifier = HIWORD(data);
            }
        }
    }

    // Update working copy
    m_configCopy.updateInterval = newInterval;
    m_configCopy.displayUnit = newUnit;
    m_configCopy.autoStart = newAutoStart;
    m_configCopy.enableLogging = newEnableLogging;
    m_configCopy.debugLogging = newDebugLogging;
    m_configCopy.enableConnectionNotification = newConnectionNotify;
    m_configCopy.darkTheme = newDarkTheme;
    m_configCopy.themeMode = newThemeMode;
    m_configCopy.selectedInterface = newInterface;
    m_configCopy.historyAutoTrimDays = newTrimDays;
    m_configCopy.language = newLanguage;
    m_configCopy.pingTarget = newPingTarget;
    m_configCopy.pingIntervalMs = newPingInterval;
    m_configCopy.hotkeyModifier = newHotkeyModifier;
    m_configCopy.hotkeyKey = newHotkeyKey;

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
