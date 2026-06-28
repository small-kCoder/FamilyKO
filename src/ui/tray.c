/* UI: 系统托盘 */
#include "ui/tray.h"
#include "common.h"

#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

#define ID_TRAY  2001
#define ID_SHOW  2002
#define ID_EXIT  2003

static NOTIFYICONDATAW g_nid = {0};
static HMENU g_menu = NULL;
static HINSTANCE g_inst = NULL;
static bool g_inited = false;

bool pcretro_tray_init(HINSTANCE hInst) {
    g_inst = hInst;
    g_menu = CreatePopupMenu();
    AppendMenuW(g_menu, MF_STRING, ID_SHOW, L"打开主界面");
    AppendMenuW(g_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_menu, MF_STRING, ID_EXIT, L"退出");
    g_inited = true;
    return true;
}

void pcretro_tray_shutdown(void) {
    if (g_menu) { DestroyMenu(g_menu); g_menu = NULL; }
    g_inited = false;
}

bool pcretro_tray_add(HWND hwnd) {
    if (!g_inited) return false;
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = hwnd;
    g_nid.uID              = ID_TRAY;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER_TRAY;
    g_nid.hIcon            = LoadIconW(g_inst ? g_inst : NULL, IDI_APPLICATION);
    wcscpy(g_nid.szTip, L"电脑回溯器（家长管控版）");
    return Shell_NotifyIconW(NIM_ADD, &g_nid) != FALSE;
}

void pcretro_tray_remove(void) {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void pcretro_tray_show_balloon(const wchar_t* title, const wchar_t* text) {
    g_nid.uFlags |= NIF_INFO;
    wcsncpy(g_nid.szInfoTitle, title ? title : L"PCRetro", 63);
    wcsncpy(g_nid.szInfo, text ? text : L"", 255);
    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags &= ~NIF_INFO;
}

LRESULT pcretro_tray_on_message(HWND hwnd, LPARAM lParam) {
    switch (LOWORD(lParam)) {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONUP: {
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenu(g_menu, TPM_RETURNCMD | TPM_NONOTIFY,
            pt.x, pt.y, 0, hwnd, NULL);
        if (cmd == ID_SHOW) {
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
        } else if (cmd == ID_EXIT) {
            DestroyWindow(hwnd);
        }
        break;
    }
    }
    return 0;
}
