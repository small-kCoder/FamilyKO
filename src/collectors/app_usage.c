/* 应用使用采集：通过 GetForegroundWindow + psapi/TlHelp32 */
#include "app_usage.h"
#include "../common.h"
#include "../storage/repo.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <process.h>

#pragma comment(lib, "psapi.lib")

static volatile LONG g_running = 0;
static HANDLE g_thread = NULL;
static CRITICAL_SECTION g_lock;

typedef struct {
    DWORD pid;
    wchar_t name[MAX_PATH];
    wchar_t title[256];
    int64_t start_ts;
    int64_t last_seen_ts;
} session_t;

#define MAX_SESSIONS 32
static session_t g_sessions[MAX_SESSIONS];
static int g_session_count = 0;

static const wchar_t* SKIP[] = {
    L"svchost.exe", L"csrss.exe", L"wininit.exe", L"winlogon.exe",
    L"dwm.exe", L"explorer.exe", L"taskhostw.exe", L"sihost.exe",
    L"fontdrvhost.exe", L"WmiPrvSE.exe", L"audiodg.exe", L"smartscreen.exe",
    NULL
};

static bool is_skipped(const wchar_t* name) {
    for (int i = 0; SKIP[i]; i++) {
        if (_wcsicmp(name, SKIP[i]) == 0) return true;
    }
    return false;
}

static DWORD get_foreground_pid(void) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return 0;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return pid;
}

static bool get_process_info(DWORD pid, wchar_t* name_out, wchar_t* title_out) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!h) return false;
    HMODULE mods[1];
    DWORD needed;
    if (!EnumProcessModules(h, mods, sizeof(mods), &needed)) {
        CloseHandle(h);
        return false;
    }
    GetModuleBaseNameW(h, mods[0], name_out, MAX_PATH);
    CloseHandle(h);

    /* 标题 */
    HWND fg = GetForegroundWindow();
    wchar_t buf[256];
    GetWindowTextW(fg, buf, 256);
    wcsncpy(title_out, buf, 255);
    title_out[255] = 0;
    return true;
}

static session_t* find_session(DWORD pid, const wchar_t* name) {
    for (int i = 0; i < g_session_count; i++) {
        if (g_sessions[i].pid == pid) return &g_sessions[i];
    }
    if (g_session_count < MAX_SESSIONS) {
        session_t* s = &g_sessions[g_session_count++];
        s->pid = pid;
        wcsncpy(s->name, name, MAX_PATH - 1);
        s->title[0] = 0;
        s->start_ts = s->last_seen_ts = pcretro_now();
        return s;
    }
    return NULL;
}

static void tick(void) {
    DWORD pid = get_foreground_pid();
    if (!pid) return;
    wchar_t name[MAX_PATH] = {0}, title[256] = {0};
    if (!get_process_info(pid, name, title)) return;
    if (is_skipped(name) || name[0] == 0) return;
    EnterCriticalSection(&g_lock);
    session_t* s = find_session(pid, name);
    if (s) {
        int64_t now = pcretro_now();
        s->last_seen_ts = now;
        if (title[0] && wcscmp(title, s->title) != 0) {
            wcsncpy(s->title, title, 255);
            s->title[255] = 0;
            char* name_a = pcretro_wide_to_utf8(s->name);
            char* title_a = pcretro_wide_to_utf8(s->title);
            pcretro_repo_insert_event("app", name_a, title_a,
                "{\"event\":\"title_change\"}", 0, 0);
            free(name_a); free(title_a);
        }
    }
    LeaveCriticalSection(&g_lock);
}

static void flush_finished(void) {
    int64_t now = pcretro_now();
    EnterCriticalSection(&g_lock);
    int i = 0;
    while (i < g_session_count) {
        if (now - g_sessions[i].last_seen_ts > 5) {
            int64_t dur = (g_sessions[i].last_seen_ts - g_sessions[i].start_ts) * 1000;
            char* name_a = pcretro_wide_to_utf8(g_sessions[i].name);
            char* title_a = pcretro_wide_to_utf8(g_sessions[i].title);
            pcretro_repo_insert_event("app", name_a, title_a,
                "{\"event\":\"end\"}", (int)dur, 0);
            free(name_a); free(title_a);
            g_sessions[i] = g_sessions[--g_session_count];
        } else {
            i++;
        }
    }
    LeaveCriticalSection(&g_lock);
}

static unsigned __stdcall thread_proc(void* arg) {
    (void)arg;
    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        tick();
        flush_finished();
        Sleep(2000);
    }
    return 0;
}

void pcretro_app_usage_start(void) {
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) return;
    InitializeCriticalSection(&g_lock);
    g_session_count = 0;
    g_thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc, NULL, 0, NULL);
    pcretro_info(L"app_usage collector started");
}

void pcretro_app_usage_stop(void) {
    if (InterlockedExchange(&g_running, 0) == 0) return;
    if (g_thread) {
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
    DeleteCriticalSection(&g_lock);
    pcretro_info(L"app_usage collector stopped");
}
