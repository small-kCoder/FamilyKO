/* 锁定 */
#ifndef PCRETRO_LOCKOUT_H
#define PCRETRO_LOCKOUT_H

#include "../common.h"

bool pcretro_lockout_is_locked(int* remain_sec);
void pcretro_lockout_record_failure(void);
void pcretro_lockout_reset(void);

#endif
