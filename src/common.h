/* 公共头文件：路径、字符串、时间、原子 */
#ifndef PCRETRO_COMMON_H
#define PCRETRO_COMMON_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 抑制 MSVC 警告：stdint.h 的 INT_MIN 等 */
#if defined(_MSC_VER)
#pragma warning(disable: 5105)
#endif

/* 通用宏 */
#define PCRETRO_VERSION     "1.0.0"
#define PCRETRO_APP_NAME    "PCRetro"
#define PCRETRO_DISPLAY     "电脑回溯器"
#define PCRETRO_SERVICE     "PCRetroWatcher"
#define PCRETRO_SERVICE_W   L"PCRetroWatcher"
#define PCRETRO_HOTKEY_ID   0xB17
#define WM_USER_TRAY        (WM_USER + 1)
#define WM_USER_LOGIN_OK    (WM_USER + 2)

/* 数据目录：%ProgramData%\PCRetro */
wchar_t* pcretro_data_dir(void);
wchar_t* pcretro_db_path(void);
wchar_t* pcretro_install_dir(void);
wchar_t* pcretro_log_path(void);

/* 确保目录存在 */
bool pcretro_ensure_dir(const wchar_t* path);

/* 时间 */
int64_t pcretro_now(void);                 /* unix 秒 */
int64_t pcretro_now_ms(void);              /* unix 毫秒 */
void pcretro_localtime_str(int64_t ts, wchar_t* buf, size_t buflen); /* "YYYY-MM-DD HH:MM:SS" */

/* UTF-8 <-> wchar_t */
wchar_t* pcretro_utf8_to_wide(const char* utf8);
char* pcretro_wide_to_utf8(const wchar_t* w);

/* 日志 */
void pcretro_log(const wchar_t* level, const wchar_t* fmt, ...);
#define pcretro_info(...)  pcretro_log(L"INFO", __VA_ARGS__)
#define pcretro_warn(...)  pcretro_log(L"WARN", __VA_ARGS__)
#define pcretro_error(...) pcretro_log(L"ERROR", __VA_ARGS__)

/* 字符串辅助 */
wchar_t* pcretro_wcsdup(const wchar_t* s);
char* pcretro_strdup_a(const char* s);

/* 进程工具 */
DWORD pcretro_current_pid(void);
bool pcretro_is_admin(void);
bool pcretro_run_as_admin_if_needed(void);

#endif
