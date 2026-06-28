/* UI: AI 总结视图 */
#ifndef PCRETRO_UI_AI_VIEW_H
#define PCRETRO_UI_AI_VIEW_H

#include <windows.h>

HWND pcretro_ai_view_create(HWND parent);
void pcretro_ai_view_refresh(HWND self, const wchar_t* range);
void pcretro_ai_view_cancel(HWND self);

#endif
