/* 全局热键 Ctrl+Alt+P */
#ifndef PCRETRO_HOTKEY_H
#define PCRETRO_HOTKEY_H

#include "../common.h"

typedef void (*pcretro_hotkey_callback)(void);

bool pcretro_hotkey_register(HWND hwnd, pcretro_hotkey_callback cb);
void pcretro_hotkey_unregister(HWND hwnd);

#endif
