/* UI: 系统托盘 */
#ifndef PCRETRO_UI_TRAY_H
#define PCRETRO_UI_TRAY_H

#include <windows.h>
#include <stdbool.h>

bool pcretro_tray_init(HINSTANCE hInst);
void pcretro_tray_shutdown(void);
void pcretro_tray_show_balloon(const wchar_t* title, const wchar_t* text);
bool pcretro_tray_add(HWND hwnd);
void pcretro_tray_remove(void);

/* 托盘消息处理（在主窗口 proc 中转发 WM_USER_TRAY） */
LRESULT pcretro_tray_on_message(HWND hwnd, LPARAM lParam);

#endif
