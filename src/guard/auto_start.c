/* 开机自启：HKCU\Software\Microsoft\Windows\CurrentVersion\Run */
#include "auto_start.h"
#include "../common.h"
#include <shlobj.h>

#define KEY_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define VALUE_NAME L"PCRetroMonitor"

static bool get_command(wchar_t* out, int cap) {
    /* 优先取当前 exe */
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    /* 如果是 PCRetroInstaller.exe 启动 UI，否则直接用当前 */
    _snwprintf(out, cap, L"\"%s\"", exe);
    return true;
}

bool pcretro_autostart_enable(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_PATH, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return false;
    wchar_t cmd[MAX_PATH * 2];
    get_command(cmd, MAX_PATH * 2);
    LONG r = RegSetValueExW(hKey, VALUE_NAME, 0, REG_SZ, (BYTE*)cmd, (DWORD)wcslen(cmd) * 2);
    RegCloseKey(hKey);
    return r == ERROR_SUCCESS;
}

bool pcretro_autostart_disable(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_PATH, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return false;
    LONG r = RegDeleteValueW(hKey, VALUE_NAME);
    RegCloseKey(hKey);
    return r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND;
}

bool pcretro_autostart_is_enabled(void) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_PATH, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return false;
    wchar_t buf[MAX_PATH];
    DWORD sz = sizeof(buf), type = 0;
    LONG r = RegQueryValueExW(hKey, VALUE_NAME, NULL, &type, (BYTE*)buf, &sz);
    RegCloseKey(hKey);
    return r == ERROR_SUCCESS;
}
