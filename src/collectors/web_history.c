/* 浏览器历史采集：拷贝 SQLite 到临时目录只读打开 */
#include "web_history.h"
#include "../common.h"
#include "../storage/repo.h"
#include "../storage/db.h"
#include <shlobj.h>
#include <process.h>

static volatile LONG g_running = 0;
static HANDLE g_thread = NULL;

static void copy_to_temp(const wchar_t* src, wchar_t* dst, size_t cap) {
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    _snwprintf(dst, cap, L"%spcretro_wh_%d.db", tmp, GetCurrentProcessId());
    CopyFileW(src, dst, FALSE);
}

static int parse_chromium(const wchar_t* src, const char* browser) {
    wchar_t tmp[MAX_PATH];
    copy_to_temp(src, tmp, MAX_PATH);
    char tmp_a[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, tmp, -1, tmp_a, MAX_PATH, NULL, NULL);

    sqlite3* h = NULL;
    if (sqlite3_open_v2(tmp_a, &h, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        sqlite3_close(h);
        DeleteFileW(tmp);
        return 0;
    }
    int64_t since = pcretro_now() - 24 * 3600;
    int64_t webkit = (since + 11644473600) * 1000000;
    sqlite3_stmt* st = NULL;
    const char* sql = "SELECT url, title, last_visit_time FROM urls "
                      "WHERE last_visit_time >= ? ORDER BY last_visit_time DESC LIMIT 500";
    if (sqlite3_prepare_v2(h, sql, -1, &st, NULL) != SQLITE_OK) {
        sqlite3_close(h); DeleteFileW(tmp); return 0;
    }
    sqlite3_bind_int64(st, 1, webkit);
    int n = 0;
    pcretro_db_t* db = pcretro_db_thread();
    pcretro_db_begin(db);
    while (sqlite3_step(st) == SQLITE_ROW) {
        const char* url = (const char*)sqlite3_column_text(st, 0);
        const char* title = (const char*)sqlite3_column_text(st, 1);
        int64_t lv = sqlite3_column_int64(st, 2);
        if (!url) continue;
        int64_t ts = (lv / 1000000) - 11644473600;
        if (ts < since) continue;
        char payload[2048];
        _snprintf(payload, sizeof(payload), "{\"url\":\"%s\",\"browser\":\"%s\"}",
            url, browser);
        pcretro_repo_insert_event("web", browser, title ? title : "", payload, 0, 0);
        n++;
    }
    pcretro_db_commit(db);
    sqlite3_finalize(st);
    sqlite3_close(h);
    DeleteFileW(tmp);
    return n;
}

static int parse_firefox(const wchar_t* src) {
    wchar_t tmp[MAX_PATH];
    copy_to_temp(src, tmp, MAX_PATH);
    char tmp_a[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, tmp, -1, tmp_a, MAX_PATH, NULL, NULL);

    sqlite3* h = NULL;
    if (sqlite3_open_v2(tmp_a, &h, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        sqlite3_close(h); DeleteFileW(tmp); return 0;
    }
    int64_t since = pcretro_now() - 24 * 3600;
    int64_t moz = since * 1000000;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(h,
        "SELECT url, title, last_visit_date FROM moz_places "
        "WHERE last_visit_date >= ? ORDER BY last_visit_date DESC LIMIT 500",
        -1, &st, NULL) != SQLITE_OK) {
        sqlite3_close(h); DeleteFileW(tmp); return 0;
    }
    sqlite3_bind_int64(st, 1, moz);
    int n = 0;
    pcretro_db_t* db = pcretro_db_thread();
    pcretro_db_begin(db);
    while (sqlite3_step(st) == SQLITE_ROW) {
        const char* url = (const char*)sqlite3_column_text(st, 0);
        const char* title = (const char*)sqlite3_column_text(st, 1);
        int64_t lv = sqlite3_column_int64(st, 2);
        if (!url || !lv) continue;
        int64_t ts = lv / 1000000;
        if (ts < since) continue;
        char payload[2048];
        _snprintf(payload, sizeof(payload), "{\"url\":\"%s\",\"browser\":\"firefox\"}", url);
        pcretro_repo_insert_event("web", "firefox", title ? title : "", payload, 0, 0);
        n++;
    }
    pcretro_db_commit(db);
    sqlite3_finalize(st);
    sqlite3_close(h);
    DeleteFileW(tmp);
    return n;
}

static void glob_one(const wchar_t* pattern, void (*cb)(const wchar_t*)) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) cb(fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static int poll_once(void) {
    int total = 0;
    wchar_t user[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, user);

    wchar_t pat[MAX_PATH];
    /* Chrome */
    _snwprintf(pat, MAX_PATH, L"%s\\AppData\\Local\\Google\\Chrome\\User Data\\*\\History", user);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            wchar_t full[MAX_PATH];
            _snwprintf(full, MAX_PATH, L"%s\\AppData\\Local\\Google\\Chrome\\User Data\\%s\\History", user, fd.cFileName);
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                total += parse_chromium(full, "chrome");
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    /* Edge */
    _snwprintf(pat, MAX_PATH, L"%s\\AppData\\Local\\Microsoft\\Edge\\User Data\\*\\History", user);
    h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            wchar_t full[MAX_PATH];
            _snwprintf(full, MAX_PATH, L"%s\\AppData\\Local\\Microsoft\\Edge\\User Data\\%s\\History", user, fd.cFileName);
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                total += parse_chromium(full, "edge");
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    /* Firefox */
    _snwprintf(pat, MAX_PATH, L"%s\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles\\*\\places.sqlite", user);
    h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            wchar_t full[MAX_PATH];
            _snwprintf(full, MAX_PATH, L"%s\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles\\%s\\places.sqlite", user, fd.cFileName);
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                total += parse_firefox(full);
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    return total;
}

static unsigned __stdcall thread_proc(void* arg) {
    (void)arg;
    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        poll_once();
        Sleep(300000); /* 5 分钟 */
    }
    return 0;
}

int pcretro_web_history_poll_once(void) { return poll_once(); }

void pcretro_web_history_start(void) {
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) return;
    g_thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc, NULL, 0, NULL);
}

void pcretro_web_history_stop(void) {
    if (InterlockedExchange(&g_running, 0) == 0) return;
    if (g_thread) {
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
}
