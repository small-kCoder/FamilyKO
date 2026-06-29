/* UI: 主窗口（标签页容器） */
#ifndef PCRETRO_UI_MAIN_WINDOW_H
#define PCRETRO_UI_MAIN_WINDOW_H

#include <windows.h>
#include <stdbool.h>

/* 创建主窗口（不显示） */
HWND pcretro_main_window_create(HINSTANCE hInst);

/* 弹出登录窗口，成功返回 true */
bool pcretro_main_window_do_login(HWND hwnd);

/* 切换标签页 */
void pcretro_main_window_show_tab(int idx);

/* 主窗口消息处理 */
LRESULT CALLBACK pcretro_main_window_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

#endif
