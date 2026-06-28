/* UI: 主窗口（TabView 容器） */
#include "ui/main_window.h"
#include "ui/login.h"
#include "ui/event_view.h"
#include "ui/stats_view.h"
#include "ui/ai_view.h"
#include "ui/control_view.h"
#include "ui/settings_view.h"
#include "ui/tray.h"
#include "common.h"
#include "security/hotkey.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#pragma comment(lib, "comctl32.lib")

#define ID_TAB        1001
#define ID_STATUSBAR  1003

/* 子视图句柄 */
static HWND g_tabs[8] = {0};
static int  g_tab_count = 0;
static HWND g_statusbar = NULL;
static HWND g_tab = NULL;
static HINSTANCE g_inst = NULL;

/* 状态栏：显示当前时间和事件数 */
static void update_statusbar(void) {
    wchar_t txt[128];
    wchar_t ts[32];
    pcretro_localtime_str(pcretro_now(), ts, 32);
    _snwprintf(txt, 128, L"时间：%s  |  监控中", ts);
    SendMessageW(g_statusbar, SB_SETTEXTW, 0, (LPARAM)txt);
}

/* 标签页选择变化 */
static void on_tab_change(int idx) {
    for (int i = 0; i < g_tab_count; i++) {
        ShowWindow(g_tabs[i], (i == idx) ? SW_SHOW : SW_HIDE);
    }
    if (idx == 0)      pcretro_event_view_refresh(g_tabs[0], L"今天");
    else if (idx == 1) pcretro_stats_view_refresh(g_tabs[1], L"今天");
    else if (idx == 2) pcretro_ai_view_refresh(g_tabs[2], L"今天");
    else if (idx == 3) pcretro_control_view_refresh(g_tabs[3]);
    else if (idx == 4) pcretro_settings_view_refresh(g_tabs[4]);
}

/* 全局热键回调：Ctrl+Alt+P 拉起主窗口 */
static HWND g_hwnd_main = NULL;
static void on_hotkey_show(void) {
    if (g_hwnd_main) {
        ShowWindow(g_hwnd_main, SW_SHOWNORMAL);
        SetForegroundWindow(g_hwnd_main);
    }
}

LRESULT CALLBACK pcretro_main_window_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd_main = hwnd;
        INITCOMMONCONTROLSEX icc = {0};
        icc.dwSize = sizeof(icc);
        icc.dwICC  = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icc);

        g_tab = CreateWindowExW(0, WC_TABCONTROLW, NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 100, 30, hwnd, (HMENU)ID_TAB, g_inst, NULL);

        TCITEMW ti = {0};
        ti.mask = TCIF_TEXT;
        const wchar_t* titles[] = { L"事件流", L"统计", L"AI 总结", L"家长控制", L"设置" };
        int n = sizeof(titles) / sizeof(titles[0]);
        for (int i = 0; i < n; i++) {
            ti.pszText = (LPWSTR)titles[i];
            TabCtrl_InsertItem(g_tab, i, &ti);
        }
        g_tab_count = n;

        RECT rc;
        GetClientRect(hwnd, &rc);
        TabCtrl_AdjustRect(g_tab, FALSE, &rc);

        g_tabs[0] = pcretro_event_view_create(hwnd);
        g_tabs[1] = pcretro_stats_view_create(hwnd);
        g_tabs[2] = pcretro_ai_view_create(hwnd);
        g_tabs[3] = pcretro_control_view_create(hwnd);
        g_tabs[4] = pcretro_settings_view_create(hwnd);
        for (int i = 0; i < g_tab_count; i++) {
            MoveWindow(g_tabs[i], rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            ShowWindow(g_tabs[i], SW_HIDE);
        }
        ShowWindow(g_tabs[0], SW_SHOW);

        g_statusbar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, (HMENU)ID_STATUSBAR, g_inst, NULL);
        update_statusbar();
        SetTimer(hwnd, 1, 30000, NULL);

        pcretro_hotkey_register(hwnd, on_hotkey_show);
        pcretro_tray_add(hwnd);
        break;
    }
    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        MoveWindow(g_tab, 0, 0, rc.right, rc.bottom - 24);
        TabCtrl_AdjustRect(g_tab, FALSE, &rc);
        for (int i = 0; i < g_tab_count; i++) {
            MoveWindow(g_tabs[i], rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }
        SendMessageW(g_statusbar, WM_SIZE, 0, 0);
        break;
    }
    case WM_TIMER:
        update_statusbar();
        break;
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)l;
        if (nm->idFrom == ID_TAB && nm->code == TCN_SELCHANGE) {
            int idx = TabCtrl_GetCurSel(g_tab);
            on_tab_change(idx);
        }
        break;
    }
    case WM_HOTKEY:
        pcretro_hotkey_on_hotkey_msg(w);
        break;
    case WM_USER_TRAY:
        return pcretro_tray_on_message(hwnd, l);
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        pcretro_tray_show_balloon(L"PCRetro", L"窗口已隐藏到托盘，使用 Ctrl+Alt+P 重新打开。");
        return 0;
    case WM_DESTROY:
        pcretro_hotkey_unregister(hwnd);
        pcretro_tray_remove();
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

HWND pcretro_main_window_create(HINSTANCE hInst) {
    g_inst = hInst;
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = pcretro_main_window_proc;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIconW(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PCRetroMainWindow";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"电脑回溯器（家长管控版）",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        NULL, NULL, hInst, NULL);
    return hwnd;
}

bool pcretro_main_window_do_login(HWND hwnd) {
    (void)hwnd;
    return pcretro_login_dialog_show(hwnd);
}

void pcretro_main_window_show_tab(int idx) {
    if (idx >= 0 && idx < g_tab_count) {
        TabCtrl_SetCurSel(g_tab, idx);
        on_tab_change(idx);
    }
}
