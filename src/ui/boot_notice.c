/* UI: 启动通知对话框 */
#include "ui/boot_notice.h"
#include "common.h"

#include <windows.h>
#include <stdio.h>

#define ID_AGREE   3001
#define ID_DISAGREE 3002

static bool g_agree = false;
static HWND g_dlg = NULL;

static LRESULT CALLBACK notice_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)l;
    switch (msg) {
    case WM_CREATE: {
        CreateWindowExW(0, L"STATIC",
            L"电脑回溯器（家长管控版）\r\n\r\n"
            L"本软件用于家长对孩子电脑使用进行合法、透明的管理。\r\n\r\n"
            L"本软件运行时会记录：\r\n"
            L"  • 打开的应用程序和使用时长\r\n"
            L"  • 浏览器访问的网页标题和地址\r\n"
            L"  • 桌面、文档、下载、图片中的文件操作\r\n\r\n"
            L"如需停止记录，请联系家长。\r\n"
            L"点击「我已知晓」表示你已了解本次会话会被记录。",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 20, 460, 220, hwnd, NULL, NULL, NULL);
        CreateWindowExW(0, L"BUTTON", L"我已知晓",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            250, 260, 100, 32, hwnd, (HMENU)ID_AGREE, NULL, NULL);
        CreateWindowExW(0, L"BUTTON", L"退出",
            WS_CHILD | WS_VISIBLE,
            360, 260, 100, 32, hwnd, (HMENU)ID_DISAGREE, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == ID_AGREE) {
            g_agree = true;
            DestroyWindow(hwnd);
        } else if (LOWORD(w) == ID_DISAGREE) {
            g_agree = false;
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        g_agree = false;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

bool pcretro_boot_notice_show(HINSTANCE hInst) {
    (void)hInst;
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = notice_proc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PCRetroBootNotice";
    RegisterClassExW(&wc);

    g_agree = false;
    g_dlg = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName,
        L"会话记录通知",
        WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 340,
        NULL, NULL, wc.hInstance, NULL);
    ShowWindow(g_dlg, SW_SHOW);
    UpdateWindow(g_dlg);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(g_dlg)) break;
    }
    return g_agree;
}
