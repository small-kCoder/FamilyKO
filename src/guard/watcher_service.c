/* Windows 服务入口：监视并自动重启 PCRetroUI.exe */
#include "common.h"
#include "guard/installer.h"

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <psapi.h>

/* ---- 服务状态 ---- */
static SERVICE_STATUS          g_ss = {0};
static SERVICE_STATUS_HANDLE   g_ssh = NULL;
static HANDLE                  g_stop_evt = NULL;

/* 监视目标可执行名 */
static const wchar_t* TARGET_EXE = L"PCRetroUI.exe";

/* 进程名是否在运行 */
static bool process_exists(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = {0};
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

/* 拉起 UI 进程 */
static void launch_ui(void) {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) *(slash + 1) = L'\0';
    wcsncat(path, TARGET_EXE, MAX_PATH - wcslen(path) - 1);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    if (CreateProcessW(path, NULL, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        pcretro_info(L"已拉起 UI：%s", path);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        pcretro_error(L"无法拉起 UI，错误码=%lu", GetLastError());
    }
}

/* 报告服务状态 */
static void report_state(DWORD state, DWORD exit_code, DWORD wait_hint) {
    g_ss.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    g_ss.dwCurrentState            = state;
    g_ss.dwWin32ExitCode           = exit_code;
    g_ss.dwWaitHint                = wait_hint;
    g_ss.dwControlsAccepted        = (state == SERVICE_START_PENDING) ? 0
                                      : SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    if (state == SERVICE_RUNNING) g_ss.dwCheckPoint = 0;
    else g_ss.dwCheckPoint++;
    SetServiceStatus(g_ssh, &g_ss);
}

/* 服务控制处理 */
static void WINAPI svc_ctrl(DWORD ctrl) {
    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        report_state(SERVICE_STOP_PENDING, NO_ERROR, 3000);
        SetEvent(g_stop_evt);
        break;
    case SERVICE_INTERROGATE:
        SetServiceStatus(g_ssh, &g_ss);
        break;
    default: break;
    }
}

/* 服务主循环：每 5 秒检查 UI 进程；不在则拉起 */
static void WINAPI svc_main(DWORD argc, wchar_t** argv) {
    (void)argc; (void)argv;
    g_ssh = RegisterServiceCtrlHandlerW(PCRETRO_SERVICE_W, svc_ctrl);
    if (!g_ssh) return;

    g_stop_evt = CreateEventW(NULL, TRUE, FALSE, NULL);
    report_state(SERVICE_START_PENDING, NO_ERROR, 1000);

    pcretro_info(L"PCRetroWatcher 服务启动");
    report_state(SERVICE_RUNNING, NO_ERROR, 0);

    /* 首次启动时拉起 UI */
    if (!process_exists(TARGET_EXE)) launch_ui();

    while (WaitForSingleObject(g_stop_evt, 5000) == WAIT_TIMEOUT) {
        if (!process_exists(TARGET_EXE)) {
            pcretro_warn(L"UI 进程已退出，将自动重启");
            launch_ui();
        }
    }

    pcretro_info(L"PCRetroWatcher 服务停止");
    report_state(SERVICE_STOPPED, NO_ERROR, 0);
    if (g_stop_evt) CloseHandle(g_stop_evt);
    g_stop_evt = NULL;
}

/* 控制台模式：按需以普通进程方式运行相同循环（用于调试） */
static int run_console(void) {
    pcretro_info(L"PCRetroWatcher 控制台模式启动");
    g_stop_evt = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE sig = CreateEventW(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler(NULL, FALSE);
    if (!process_exists(TARGET_EXE)) launch_ui();
    while (WaitForSingleObject(sig, 5000) == WAIT_TIMEOUT) {
        if (!process_exists(TARGET_EXE)) launch_ui();
    }
    if (g_stop_evt) CloseHandle(g_stop_evt);
    if (sig) CloseHandle(sig);
    return 0;
}

int wmain(int argc, wchar_t** argv) {
    /* 参数：service 表示以服务方式运行；其他=控制台模式 */
    bool as_service = (argc >= 2 && _wcsicmp(argv[1], L"service") == 0);
    if (as_service) {
        SERVICE_TABLE_ENTRYW table[] = {
            { (LPWSTR)PCRETRO_SERVICE_W, svc_main },
            { NULL, NULL }
        };
        if (!StartServiceCtrlDispatcherW(table)) {
            /* 不是 SCM 启动 → 退化到控制台 */
            return run_console();
        }
        return 0;
    }
    return run_console();
}
