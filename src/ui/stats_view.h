/* UI: 统计视图 */
#ifndef PCRETRO_UI_STATS_VIEW_H
#define PCRETRO_UI_STATS_VIEW_H

#include <windows.h>

HWND pcretro_stats_view_create(HWND parent);
void pcretro_stats_view_refresh(HWND self, const wchar_t* range);

#endif
