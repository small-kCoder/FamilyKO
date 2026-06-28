/* UI: 登录对话框 */
#ifndef PCRETRO_UI_LOGIN_H
#define PCRETRO_UI_LOGIN_H

#include <windows.h>
#include <stdbool.h>

/* 阻塞式登录窗口：返回 true 表示已通过 */
bool pcretro_login_dialog_show(HWND parent);

#endif
