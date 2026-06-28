/* UI: 设置视图 */
#include "ui/settings_view.h"
#include "ui/dialogs.h"
#include "common.h"
#include "storage/repo.h"
#include "guard/auto_start.h"
#include "guard/installer.h"

#include <windows.h>
#include <stdio.h>

#define IDC_BTN_SETPWD 8001
#define IDC_BTN_CHGPWD 8002
#define IDC_BTN_AUTOSTART 8003
#define IDC_BTN_INSTSRV 8004
#define IDC_BTN_UNINSTSRV 8005
#define IDC_BTN_START 8006
#define IDC_BTN_STOP 8007
#define IDC_BTN_CLEAN 8008
#define IDC_VER 8009
#define IDC_DBINFO 8010

static HWND g_root = NULL;

static LRESULT CALLBACK root_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)l;
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(w);
        if      (id == IDC_BTN_SETPWD)    pcretro_dlg_set_password(hwnd);
        else if (id == IDC_BTN_CHGPWD)    pcretro_dlg_change_password(hwnd);
        else if (id == IDC_BTN_AUTOSTART) {
            bool en = !pcretro_autostart_is_enabled();
            if (en) pcretro_autostart_enable(); else pcretro_autostart_disable();
            MessageBoxW(hwnd, en ? L"已设置开机启动" : L"已取消开机启动",
                L"PCRetro", MB_ICONINFORMATION);
        }
        else if (id == IDC_BTN_INSTSRV) {
            if (pcretro_service_install())
                MessageBoxW(hwnd, L"服务已安装", L"PCRetro", MB_OK);
            else
                MessageBoxW(hwnd, L"服务安装失败（需管理员）", L"PCRetro", MB_ICONERROR);
        }
        else if (id == IDC_BTN_UNINSTSRV)
            pcretro_service_uninstall();
        else if (id == IDC_BTN_START) pcretro_service_start();
        else if (id == IDC_BTN_STOP)  pcretro_service_stop();
        else if (id == IDC_BTN_CLEAN) {
            if (MessageBoxW(hwnd, L"确认清空所有事件记录？", L"PCRetro",
                MB_YESNO | MB_ICONWARNING) == IDYES) {
                pcretro_repo_clear_events();
            }
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

HWND pcretro_settings_view_create(HWND parent) {
    g_root = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 100, 100, parent, NULL, GetModuleHandleW(NULL), NULL);
    SetWindowLongPtrW(g_root, GWLP_WNDPROC, (LONG_PTR)root_proc);

    int y = 10;
    CreateWindowExW(0, L"STATIC", L"版本：1.0.0", WS_CHILD | WS_VISIBLE,
        10, y, 400, 22, g_root, (HMENU)IDC_VER, NULL, NULL);
    y += 30;

    CreateWindowExW(0, L"STATIC", L"—— 家长账户 ——", WS_CHILD | WS_VISIBLE,
        10, y, 300, 22, g_root, NULL, NULL, NULL);
    y += 26;
    CreateWindowExW(0, L"BUTTON", L"设置密码", WS_CHILD | WS_VISIBLE,
        10, y, 120, 28, g_root, (HMENU)IDC_BTN_SETPWD, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"修改密码", WS_CHILD | WS_VISIBLE,
        140, y, 120, 28, g_root, (HMENU)IDC_BTN_CHGPWD, NULL, NULL);
    y += 40;

    CreateWindowExW(0, L"STATIC", L"—— 启动与保护 ——", WS_CHILD | WS_VISIBLE,
        10, y, 300, 22, g_root, NULL, NULL, NULL);
    y += 26;
    CreateWindowExW(0, L"BUTTON", L"切换开机启动", WS_CHILD | WS_VISIBLE,
        10, y, 140, 28, g_root, (HMENU)IDC_BTN_AUTOSTART, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"安装 Watcher 服务", WS_CHILD | WS_VISIBLE,
        160, y, 160, 28, g_root, (HMENU)IDC_BTN_INSTSRV, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"卸载 Watcher 服务", WS_CHILD | WS_VISIBLE,
        330, y, 160, 28, g_root, (HMENU)IDC_BTN_UNINSTSRV, NULL, NULL);
    y += 40;

    CreateWindowExW(0, L"BUTTON", L"启动服务", WS_CHILD | WS_VISIBLE,
        10, y, 100, 28, g_root, (HMENU)IDC_BTN_START, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"停止服务", WS_CHILD | WS_VISIBLE,
        120, y, 100, 28, g_root, (HMENU)IDC_BTN_STOP, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"清空事件记录", WS_CHILD | WS_VISIBLE,
        230, y, 140, 28, g_root, (HMENU)IDC_BTN_CLEAN, NULL, NULL);
    y += 40;

    wchar_t info[MAX_PATH + 32];
    _snwprintf(info, MAX_PATH + 32, L"数据库：%s", pcretro_db_path());
    CreateWindowExW(0, L"STATIC", info, WS_CHILD | WS_VISIBLE,
        10, y, 800, 22, g_root, (HMENU)IDC_DBINFO, NULL, NULL);

    return g_root;
}

void pcretro_settings_view_refresh(HWND self) { (void)self; }
