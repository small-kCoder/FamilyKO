/* PCRetroUI 主入口 */
#include "common.h"
#include "storage/db.h"
#include "storage/repo.h"
#include "security/password.h"
#include "security/lockout.h"
#include "collectors/runner.h"
#include "guard/installer.h"
#include "ui/main_window.h"
#include "ui/tray.h"
#include "ui/boot_notice.h"

#include <windows.h>
#include <stdio.h>

/* 设置项：是否在启动时显示"会话记录"通知 */
#define SETTING_BOOT_NOTICE  "show_boot_notice"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmd, int show) {
    (void)hPrev; (void)cmd;

    /* 1. 启动时弹"启动通知"对话框告知孩子本次会话会被记录（首次安装时） */
    if (pcretro_repo_setting_get_int(SETTING_BOOT_NOTICE, 1) == 1) {
        if (pcretro_boot_notice_show(hInst)) {
            pcretro_repo_setting_set_int(SETTING_BOOT_NOTICE, 0);
        }
    }

    /* 2. 初始化数据目录、SQLite */
    wchar_t* ddir = pcretro_data_dir();
    pcretro_ensure_dir(ddir);

    pcretro_db_t* db = NULL;
    if (pcretro_db_open(&db) != 0 || !db) {
        MessageBoxW(NULL, L"无法初始化数据库", L"PCRetro", MB_ICONERROR);
        return 1;
    }
    pcretro_db_init_schema(db);

    /* 3. 启动采集器线程（应用使用、网页、文件） */
    pcretro_runner_start_all();

    /* 4. 启动托盘 + 主窗口（先创建但隐藏） */
    pcretro_tray_init(hInst);
    HWND main = pcretro_main_window_create(hInst);
    if (!main) {
        MessageBoxW(NULL, L"无法创建主窗口", L"PCRetro", MB_ICONERROR);
        pcretro_runner_stop_all();
        pcretro_db_close(db);
        return 1;
    }
    ShowWindow(main, SW_HIDE);

    /* 5. 弹出登录窗口（如果设置了密码） */
    if (pcretro_password_is_initialized() && !pcretro_main_window_do_login(main)) {
        /* 用户取消登录，退出 */
        pcretro_runner_stop_all();
        pcretro_tray_shutdown();
        pcretro_db_close(db);
        return 0;
    }

    /* 登录通过：显示主窗口 */
    ShowWindow(main, show);
    UpdateWindow(main);

    /* 6. 消息循环 */
    MSG msg = {0};
    BOOL ret;
    while ((ret = GetMessageW(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) break;
        if (!IsDialogMessageW(main, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    pcretro_runner_stop_all();
    pcretro_tray_shutdown();
    pcretro_db_close(db);
    return (int)msg.wParam;
}
