/* UI: 事件流视图 */
#ifndef PCRETRO_UI_EVENT_VIEW_H
#define PCRETRO_UI_EVENT_VIEW_H

#include <windows.h>

/* 在父窗口中创建事件视图控件，返回容器句柄 */
HWND pcretro_event_view_create(HWND parent);

/* 刷新事件列表（按范围） */
void pcretro_event_view_refresh(HWND self, const wchar_t* range);

#endif
