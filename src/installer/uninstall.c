/* PCRetroUninstaller */
#include "common.h"
#include "guard/installer.h"
#include "guard/auto_start.h"

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>

#define INSTALL_DIR L"C:\\Program Files\\PCRetro"

int wmain(int argc, wchar_t** argv) {
    (void)argc; (void)argv;

    wprintf(L"PCRetro 卸载程序\n");
    wprintf(L"================\n\n");

    if (!pcretro_is_admin()) {
        wprintf(L"需要管理员权限。正在请求提权…\n");
        wchar_t exe[MAX_PATH];
        GetModuleFileNameW(NULL, exe, MAX_PATH);
        SHELLEXECUTEINFOW si = {0};
        si.cbSize = sizeof(si);
        si.lpVerb = L"runas";
        si.lpFile = exe;
        si.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&si)) return 0;
        wprintf(L"提权失败，请右键以管理员身份运行。\n");
        return 2;
    }

    bool keep_data = (argc >= 2 && _wcsicmp(argv[1], L"/keep-data") == 0);
    if (!keep_data) {
        wprintf(L"将同时删除 %s 中的数据库与日志。\n", pcretro_data_dir());
        wprintf(L"如需保留，输入 K 确认（默认删除）。\n");
        wprintf(L"输入 N 保留数据。\n选择：");
        int c = _getwch();
        wprintf(L"\n");
        keep_data = (c == 'N' || c == 'n');
    }

    wprintf(L"[1/4] 停止并卸载服务…\n");
    pcretro_service_stop();
    pcretro_service_uninstall();

    wprintf(L"[2/4] 取消开机启动…\n");
    pcretro_autostart_disable();

    /* 关闭 UI 进程（如果有） */
    wprintf(L"[3/4] 关闭 UI 进程…\n");
    HWND hwnd = FindWindowW(L"PCRetroMainWindow", NULL);
    if (hwnd) {
        SendMessageW(hwnd, WM_CLOSE, 0, 0);
    }
    Sleep(1000);

    wprintf(L"[4/4] 删除安装目录与数据目录…\n");
    wchar_t cmd[1024];
    if (keep_data) {
        _snwprintf(cmd, 1024, L"cmd /C rmdir /S /Q \"%s\"", INSTALL_DIR);
    } else {
        _snwprintf(cmd, 1024, L"cmd /C rmdir /S /Q \"%s\" & rmdir /S /Q \"%s\"",
            INSTALL_DIR, pcretro_data_dir());
    }
    _wsystem(cmd);

    wprintf(L"\n卸载完成。\n");
    system("pause");
    return 0;
}
