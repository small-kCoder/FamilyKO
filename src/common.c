/* 公共实现 */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <shlobj.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

static wchar_t g_data_dir[MAX_PATH] = {0};
static wchar_t g_install_dir[MAX_PATH] = {0};
static bool g_inited = false;

static void ensure_inited(void) {
    if (g_inited) return;
    /* 允许通过环境变量覆盖：PCRETRO_DATA_DIR, PCRETRO_INSTALL_DIR */
    wchar_t env[1024];
    if (GetEnvironmentVariableW(L"PCRETRO_DATA_DIR", env, 1024) > 0) {
        wcsncpy(g_data_dir, env, MAX_PATH - 1);
    } else {
        wchar_t* pd = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_ProgramData, 0, NULL, &pd))) {
            _snwprintf(g_data_dir, MAX_PATH, L"%s\\PCRetro", pd);
            CoTaskMemFree(pd);
        } else {
            wcsncpy(g_data_dir, L"C:\\ProgramData\\PCRetro", MAX_PATH);
        }
    }
    if (GetEnvironmentVariableW(L"PCRETRO_INSTALL_DIR", env, 1024) > 0) {
        wcsncpy(g_install_dir, env, MAX_PATH - 1);
    } else {
        wcsncpy(g_install_dir, L"C:\\Program Files\\PCRetro", MAX_PATH);
    }
    pcretro_ensure_dir(g_data_dir);
    g_inited = true;
}

wchar_t* pcretro_data_dir(void) { ensure_inited(); return g_data_dir; }
wchar_t* pcretro_db_path(void) {
    static wchar_t buf[MAX_PATH];
    ensure_inited();
    _snwprintf(buf, MAX_PATH, L"%s\\db.sqlite", g_data_dir);
    return buf;
}
wchar_t* pcretro_install_dir(void) { ensure_inited(); return g_install_dir; }
wchar_t* pcretro_log_path(void) {
    static wchar_t buf[MAX_PATH];
    ensure_inited();
    _snwprintf(buf, MAX_PATH, L"%s\\pcretro.log", g_data_dir);
    return buf;
}

bool pcretro_ensure_dir(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return CreateDirectoryW(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
    }
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int64_t pcretro_now(void) { return (int64_t)time(NULL); }
int64_t pcretro_now_ms(void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return (int64_t)((u.QuadPart - 116444736000000000ULL) / 10000ULL);
}

void pcretro_localtime_str(int64_t ts, wchar_t* buf, size_t buflen) {
    time_t t = (time_t)ts;
    struct tm tm;
    localtime_s(&tm, &t);
    wcsftime(buf, buflen, L"%Y-%m-%d %H:%M:%S", &tm);
}

wchar_t* pcretro_utf8_to_wide(const char* utf8) {
    if (!utf8) return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len <= 0) return NULL;
    wchar_t* w = (wchar_t*)calloc((size_t)len, sizeof(wchar_t));
    if (!w) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, len);
    return w;
}

char* pcretro_wide_to_utf8(const wchar_t* w) {
    if (!w) return NULL;
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return NULL;
    char* s = (char*)calloc((size_t)len, 1);
    if (!s) return NULL;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s, len, NULL, NULL);
    return s;
}

void pcretro_log(const wchar_t* level, const wchar_t* fmt, ...) {
    wchar_t path[MAX_PATH];
    wcscpy(path, pcretro_log_path());
    FILE* f = _wfopen(path, L"a, ccs=UTF-8");
    if (!f) return;

    wchar_t ts[32];
    pcretro_localtime_str(pcretro_now(), ts, 32);

    va_list ap;
    va_start(ap, fmt);
    wchar_t body[2048];
    _vsnwprintf(body, 2048, fmt, ap);
    va_end(ap);

    fwprintf(f, L"[%s] %s: %s\n", ts, level, body);
    fclose(f);
}

wchar_t* pcretro_wcsdup(const wchar_t* s) {
    if (!s) return NULL;
    size_t n = wcslen(s) + 1;
    wchar_t* r = (wchar_t*)calloc(n, sizeof(wchar_t));
    if (r) wcsncpy(r, s, n);
    return r;
}

char* pcretro_strdup_a(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* r = (char*)calloc(n, 1);
    if (r) memcpy(r, s, n);
    return r;
}

DWORD pcretro_current_pid(void) { return GetCurrentProcessId(); }

bool pcretro_is_admin(void) {
    BOOL r = FALSE;
    HANDLE token;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION e;
        DWORD sz = sizeof(e);
        if (GetTokenInformation(token, TokenElevation, &e, sizeof(e), &sz)) {
            r = e.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return r == TRUE;
}

bool pcretro_run_as_admin_if_needed(void) {
    /* 仅占位：用于需要时重启为管理员 */
    return pcretro_is_admin();
}
