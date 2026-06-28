/* 文件操作监控：基于 ReadDirectoryChangesW */
#include "file_ops.h"
#include "../common.h"
#include "../storage/repo.h"
#include <process.h>

static volatile LONG g_running = 0;
static HANDLE g_thread = NULL;
static HANDLE g_dirs[8] = {0};
static OVERLAPPED g_ov[8] = {0};
static char g_buf[8][8192];
static int g_dir_count = 0;

static void insert_file_event(const wchar_t* op, const wchar_t* path) {
    char* op_a = pcretro_wide_to_utf8(op);
    char* path_a = pcretro_wide_to_utf8(path);
    const wchar_t* base = path + wcslen(path);
    while (base > path && base[-1] != L'\\' && base[-1] != L'/') base--;
    char* base_a = pcretro_wide_to_utf8(base);
    char payload[2048];
    _snprintf(payload, sizeof(payload), "{\"op\":\"%s\",\"path\":\"%s\"}", op_a, path_a);
    pcretro_repo_insert_event("file", "watchdog", base_a, payload, 0, 0);
    free(op_a); free(path_a); free(base_a);
}

static void process_changes(int idx) {
    DWORD bytes = 0;
    if (!GetOverlappedResult(g_dirs[idx], &g_ov[idx], &bytes, FALSE) || bytes == 0) return;
    char* p = g_buf[idx];
    while (p < g_buf[idx] + bytes) {
        FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)p;
        wchar_t name[MAX_PATH] = {0};
        wcsncpy(name, fni->FileName, fni->FileNameLength / 2);
        name[fni->FileNameLength / 2] = 0;
        const wchar_t* op = L"modified";
        if (fni->Action == FILE_ACTION_ADDED) op = L"created";
        else if (fni->Action == FILE_ACTION_REMOVED) op = L"deleted";
        else if (fni->Action == FILE_ACTION_RENAMED_NEW_NAME) op = L"renamed";
        insert_file_event(op, name);
        if (fni->NextEntryOffset == 0) break;
        p += fni->NextEntryOffset;
    }
    /* 重新发起监听 */
    memset(g_buf[idx], 0, sizeof(g_buf[idx]));
    ReadDirectoryChangesW(g_dirs[idx], g_buf[idx], sizeof(g_buf[idx]), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, &g_ov[idx], NULL);
}

static void watch_dir(const wchar_t* path) {
    if (g_dir_count >= 8) return;
    HANDLE h = CreateFileW(path, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    int idx = g_dir_count++;
    g_dirs[idx] = h;
    memset(&g_ov[idx], 0, sizeof(g_ov[idx]));
    ReadDirectoryChangesW(h, g_buf[idx], sizeof(g_buf[idx]), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, &g_ov[idx], NULL);
}

static unsigned __stdcall thread_proc(void* arg) {
    (void)arg;
    wchar_t user[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, user);
    const wchar_t* names[] = {L"Desktop", L"Documents", L"Downloads", L"Pictures"};
    for (int i = 0; i < 4; i++) {
        wchar_t full[MAX_PATH];
        _snwprintf(full, MAX_PATH, L"%s\\%s", user, names[i]);
        if (GetFileAttributesW(full) & FILE_ATTRIBUTE_DIRECTORY) watch_dir(full);
    }
    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        for (int i = 0; i < g_dir_count; i++) {
            DWORD bytes = 0;
            if (HasOverlappedIoCompleted(&g_ov[i])) {
                process_changes(i);
            }
        }
        Sleep(500);
    }
    for (int i = 0; i < g_dir_count; i++) {
        CancelIo(g_dirs[i]);
        CloseHandle(g_dirs[i]);
    }
    g_dir_count = 0;
    return 0;
}

void pcretro_file_ops_start(void) {
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) return;
    g_thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc, NULL, 0, NULL);
}

void pcretro_file_ops_stop(void) {
    if (InterlockedExchange(&g_running, 0) == 0) return;
    if (g_thread) {
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
}
