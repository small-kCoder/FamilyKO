/* PCRetroInstaller：纯控制台安装器
 *
 * 流程：
 *  1. 显示 DISCLAIMER，等待用户同意
 *  2. 拷贝文件到 %ProgramFiles%\PCRetro
 *  3. 创建数据目录 %ProgramData%\PCRetro
 *  4. 注册 Watcher 服务（需要管理员）
 *  5. 启动 Watcher 服务
 *  6. 拉起 UI
 */
#include "common.h"
#include "guard/installer.h"

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>

#define INSTALL_DIR L"C:\\Program Files\\PCRetro"
#define DATA_DIR    L"%ProgramData%\\PCRetro"

/* 拷贝单个文件 */
static bool copy_file(const wchar_t* src, const wchar_t* dst) {
    if (!CopyFileW(src, dst, FALSE)) {
        wprintf(L"复制失败：%s -> %s (错误码=%lu)\n", src, dst, GetLastError());
        return false;
    }
    return true;
}

/* 显示免责声明 */
static bool show_disclaimer(void) {
    FILE* f = NULL;
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) *slash = 0;
    wcsncat(path, L"\\DISCLAIMER.txt", MAX_PATH - wcslen(path) - 1);
    _wfopen_s(&f, path, L"r, ccs=UTF-8");
    if (!f) {
        wprintf(L"无法打开 DISCLAIMER.txt：%s\n", path);
    } else {
        wchar_t line[512];
        while (fgetws(line, 512, f)) wprintf(L"%s", line);
        fclose(f);
    }
    wprintf(L"\n按 Y 同意并继续，其他键取消：");
    int c = _getwch();
    wprintf(L"\n");
    return c == 'Y' || c == 'y';
}

/* 管理员提权 */
static bool relaunch_as_admin(void) {
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    SHELLEXECUTEINFOW si = {0};
    si.cbSize = sizeof(si);
    si.lpVerb = L"runas";
    si.lpFile = exe;
    si.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&si) != FALSE;
}

int wmain(int argc, wchar_t** argv) {
    (void)argc; (void)argv;

    /* 仅 /quiet 模式跳过确认；其余情况显示免责声明 */
    bool quiet = (argc >= 2 && _wcsicmp(argv[1], L"/quiet") == 0);
    if (!quiet) {
        if (!show_disclaimer()) {
            wprintf(L"已取消安装。\n");
            return 1;
        }
    }

    /* 管理员检查 */
    if (!pcretro_is_admin()) {
        wprintf(L"需要管理员权限。正在请求提权…\n");
        if (relaunch_as_admin()) {
            return 0;
        }
        wprintf(L"提权失败，请右键以管理员身份运行。\n");
        return 2;
    }

    /* 当前 exe 所在目录（应含 PCRetroUI.exe / PCRetroWatcher.exe / PCRetroUninstaller.exe） */
    wchar_t src_dir[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, src_dir, MAX_PATH);
    wchar_t* slash = wcsrchr(src_dir, L'\\');
    if (slash) *slash = 0;

    /* 步骤 1：创建安装目录 */
    wprintf(L"[1/5] 创建安装目录：%s\n", INSTALL_DIR);
    pcretro_ensure_dir(INSTALL_DIR);

    /* 步骤 2：拷贝文件 */
    wprintf(L"[2/5] 复制文件…\n");
    const wchar_t* files[] = {
        L"PCRetroUI.exe",
        L"PCRetroWatcher.exe",
        L"PCRetroUninstaller.exe",
        L"DISCLAIMER.txt"
    };
    for (int i = 0; i < 4; i++) {
        wchar_t src[MAX_PATH], dst[MAX_PATH];
        _snwprintf(src, MAX_PATH, L"%s\\%s", src_dir, files[i]);
        _snwprintf(dst, MAX_PATH, L"%s\\%s", INSTALL_DIR, files[i]);
        if (!copy_file(src, dst)) return 3;
    }

    /* 步骤 3：创建数据目录 */
    wprintf(L"[3/5] 初始化数据目录：%s\n", pcretro_data_dir());
    pcretro_ensure_dir(pcretro_data_dir());

    /* 步骤 4：注册 Watcher 服务 */
    wprintf(L"[4/5] 注册 Watcher 服务…\n");
    if (!pcretro_service_install()) {
        wprintf(L"服务注册失败。\n");
        return 4;
    }

    /* 步骤 5：启动服务并拉起 UI */
    wprintf(L"[5/5] 启动服务并打开主界面…\n");
    pcretro_service_start();

    wchar_t ui_path[MAX_PATH];
    _snwprintf(ui_path, MAX_PATH, L"%s\\PCRetroUI.exe", INSTALL_DIR);
    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    if (CreateProcessW(ui_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    wprintf(L"\n安装完成。\n");
    if (!quiet) system("pause");
    return 0;
}
