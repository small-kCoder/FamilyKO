/* UI: 登录对话框 */
#include "ui/login.h"
#include "security/password.h"
#include "security/lockout.h"
#include "common.h"

#include <windows.h>
#include <stdio.h>

#define IDC_EDIT_PWD  101
#define IDC_BTN_OK    102
#define IDC_BTN_CANCEL 103
#define IDC_LABEL     104

static HWND g_edit = NULL;
static bool g_ok = false;

/* 子窗口消息处理 */
static LRESULT CALLBACK login_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowExW(0, L"STATIC", L"请输入家长密码：",
            WS_CHILD | WS_VISIBLE,
            20, 20, 240, 20, hwnd, (HMENU)IDC_LABEL, NULL, NULL);
        g_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL,
            20, 50, 240, 26, hwnd, (HMENU)IDC_EDIT_PWD, NULL, NULL);
        CreateWindowExW(0, L"BUTTON", L"确定",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            110, 90, 70, 28, hwnd, (HMENU)IDC_BTN_OK, NULL, NULL);
        CreateWindowExW(0, L"BUTTON", L"取消",
            WS_CHILD | WS_VISIBLE,
            190, 90, 70, 28, hwnd, (HMENU)IDC_BTN_CANCEL, NULL, NULL);
        return 0;
    case WM_COMMAND:
        if (LOWORD(w) == IDC_BTN_OK) {
            wchar_t buf[128] = {0};
            GetWindowTextW(g_edit, buf, 128);

            /* 锁定检查 */
            int remain;
            if (pcretro_lockout_is_locked(&remain)) {
                wchar_t msg[128];
                _snwprintf(msg, 128, L"尝试次数过多，请在 %d 秒后重试。", remain);
                MessageBoxW(hwnd, msg, L"PCRetro", MB_ICONWARNING);
                return 0;
            }

            if (pcretro_password_verify(buf)) {
                pcretro_lockout_reset();
                g_ok = true;
                DestroyWindow(hwnd);
            } else {
                pcretro_lockout_record_failure();
                MessageBoxW(hwnd, L"密码错误", L"PCRetro", MB_ICONERROR);
                SetWindowTextW(g_edit, L"");
            }
        } else if (LOWORD(w) == IDC_BTN_CANCEL) {
            g_ok = false;
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        g_ok = false;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

bool pcretro_login_dialog_show(HWND parent) {
    (void)parent;
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = login_proc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PCRetroLogin";
    RegisterClassExW(&wc);

    g_ok = false;
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, wc.lpszClassName,
        L"登录", WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 170,
        NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    /* 自消息循环（避免阻塞主线程） */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(hwnd)) break;
    }
    return g_ok;
}
