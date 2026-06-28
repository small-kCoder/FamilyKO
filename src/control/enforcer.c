#include "enforcer.h"
#include "blacklist.h"
#include "time_limits.h"
#include "../common.h"
#include "../storage/repo.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <process.h>

static volatile LONG g_running = 0;
static HANDLE g_thread = NULL;

static const wchar_t* WHITELIST[] = {
    L"explorer.exe", L"dwm.exe", L"winlogon.exe", L"csrss.exe", L"lsass.exe",
    L"svchost.exe", L"services.exe", L"smss.exe", L"taskhostw.exe", L"sihost.exe",
    L"PCRetroUI.exe", L"PCRetroWatcher.exe", L"System", L"audiodg.exe", L"WmiPrvSE.exe",
    L"python.exe", L"cmd.exe", L"powershell.exe", L"conhost.exe",
    NULL
};

static void enforce_loop(void) {
    pcretro_blacklist_scan();
    pcretro_limit_decision_t dec;
    pcretro_time_limits_evaluate(0, &dec);
    if (dec.allowed) return;
    /* 超时：结束非白名单进程 */
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            bool in_wl = false;
            for (int i = 0; WHITELIST[i]; i++) {
                if (_wcsicmp(WHITELIST[i], pe.szExeFile) == 0) { in_wl = true; break; }
            }
            if (in_wl) continue;
            HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (h) {
                if (TerminateProcess(h, 0)) {
                    char* n = pcretro_wide_to_utf8(pe.szExeFile);
                    pcretro_repo_insert_event("limit", n ? n : "", "超时被强制结束",
                        "{\"reason\":\"time_limit\"}", 0, 0);
                    free(n);
                }
                CloseHandle(h);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
}

static unsigned __stdcall thread_proc(void* arg) {
    (void)arg;
    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        enforce_loop();
        Sleep(5000);
    }
    return 0;
}

void pcretro_enforcer_start(void) {
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) return;
    g_thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc, NULL, 0, NULL);
}

void pcretro_enforcer_stop(void) {
    if (InterlockedExchange(&g_running, 0) == 0) return;
    if (g_thread) {
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
}
