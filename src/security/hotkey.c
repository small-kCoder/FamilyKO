#include "hotkey.h"

#define MOD_CONTROL 0x0002
#define MOD_ALT     0x0001
#define VK_P        0x50
#define HK_ID       0xB17

static pcretro_hotkey_callback g_cb = NULL;

bool pcretro_hotkey_register(HWND hwnd, pcretro_hotkey_callback cb) {
    g_cb = cb;
    if (!RegisterHotKey(hwnd, HK_ID, MOD_CONTROL | MOD_ALT, VK_P)) {
        pcretro_warn(L"RegisterHotKey 失败: %d", GetLastError());
        return false;
    }
    return true;
}

void pcretro_hotkey_unregister(HWND hwnd) {
    UnregisterHotKey(hwnd, HK_ID);
}

/* 在主窗口的 WindowProc 中调用 */
void pcretro_hotkey_on_hotkey_msg(WPARAM wParam) {
    if (wParam == HK_ID && g_cb) g_cb();
}
