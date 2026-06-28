/* 黑名单：扫描 + 强制结束 */
#include "blacklist.h"
#include "../common.h"
#include "../storage/repo.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <process.h>

#pragma comment(lib, "psapi.lib")

static volatile LONG g_running = 0;
static HANDLE g_thread = NULL;

static const wchar_t* PROTECTED[] = {
    L"csrss.exe", L"winlogon.exe", L"lsass.exe", L"svchost.exe",
    L"services.exe", L"smss.exe", L"wininit.exe", L"dwm.exe",
    L"taskhostw.exe", L"sihost.exe", L"explorer.exe", L"PCRetroUI.exe",
    L"PCRetroWatcher.exe", L"System", L"audiodg.exe", L"WmiPrvSE.exe",
    L"python.exe", L"cmd.exe", L"powershell.exe", L"conhost.exe",
    NULL
};

static bool is_protected(const wchar_t* name) {
    for (int i = 0; PROTECTED[i]; i++) {
        if (_wcsicmp(name, PROTECTED[i]) == 0) return true;
    }
    return false;
}

static bool terminate(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) return false;
    BOOL ok = TerminateProcess(h, 0);
    CloseHandle(h);
    return ok == TRUE;
}

static bool name_eq_ci(const wchar_t* a, const char* b) {
    wchar_t bw[256];
    int i = 0;
    for (; b[i] && i < 255; i++) bw[i] = (wchar_t)(unsigned char)b[i];
    bw[i] = 0;
    return _wcsicmp(a, bw) == 0;
}

static int scan(void) {
    pcretro_blacklist_t* bl = NULL; int n = 0;
    pcretro_repo_blacklist_list(1, &bl, &n);
    if (n == 0) { pcretro_repo_blacklist_free(bl); return 0; }

    int killed = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) { pcretro_repo_blacklist_free(bl); return 0; }
    PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (is_protected(pe.szExeFile)) continue;
            for (int i = 0; i < n; i++) {
                if (name_eq_ci(pe.szExeFile, bl[i].process_name)) {
                    if (terminate(pe.th32ProcessID)) {
                        char* name_a = pcretro_wide_to_utf8(pe.szExeFile);
                        pcretro_repo_insert_event("blocked", name_a, "黑名单进程被阻止",
                            "{\"reason\":\"blacklist\"}", 0, 0);
                        free(name_a);
                        killed++;
                    }
                    break;
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    pcretro_repo_blacklist_free(bl);
    return killed;
}

static unsigned __stdcall thread_proc(void* arg) {
    (void)arg;
    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        scan();
        Sleep(3000);
    }
    return 0;
}

int pcretro_blacklist_scan(void) { return scan(); }

void pcretro_blacklist_start(void) {
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) return;
    g_thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc, NULL, 0, NULL);
}

void pcretro_blacklist_stop(void) {
    if (InterlockedExchange(&g_running, 0) == 0) return;
    if (g_thread) {
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
}
