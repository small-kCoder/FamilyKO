/* 密码：bcrypt-like 哈希（用 Windows BCrypt API 实现 PBKDF2） */
#ifndef PCRETRO_PASSWORD_H
#define PCRETRO_PASSWORD_H

#include "../common.h"

bool pcretro_password_is_strong(const wchar_t* password);
int pcretro_password_set(const wchar_t* password);
bool pcretro_password_verify(const wchar_t* password);
bool pcretro_password_is_initialized(void);

#endif
