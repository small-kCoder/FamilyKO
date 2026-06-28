/* UI: 家长控制视图（黑名单/时间限制） */
#ifndef PCRETRO_UI_CONTROL_VIEW_H
#define PCRETRO_UI_CONTROL_VIEW_H

#include <windows.h>

HWND pcretro_control_view_create(HWND parent);
void pcretro_control_view_refresh(HWND self);

#endif
