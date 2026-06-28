/* UI: 通用对话框 */
#ifndef PCRETRO_UI_DIALOGS_H
#define PCRETRO_UI_DIALOGS_H

#include <windows.h>
#include <stdbool.h>

/* 输入密码（首次设置时使用） */
bool pcretro_dlg_set_password(HWND parent);

/* 修改密码 */
bool pcretro_dlg_change_password(HWND parent);

/* 添加应用黑名单 */
bool pcretro_dlg_add_blacklist(HWND parent, wchar_t* out_name, size_t out_len);

/* 添加/编辑时间限制 */
bool pcretro_dlg_edit_time_limit(HWND parent, void* limit_inout);

#endif
