/* UI: 启动通知对话框 */
#ifndef PCRETRO_UI_BOOT_NOTICE_H
#define PCRETRO_UI_BOOT_NOTICE_H

#include <windows.h>
#include <stdbool.h>

/* 显示"本次会话会被记录"通知；返回 true 表示用户已确认 */
bool pcretro_boot_notice_show(HINSTANCE hInst);

#endif
