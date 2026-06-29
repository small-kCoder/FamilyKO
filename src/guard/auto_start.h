/* 开机自启 */
#ifndef PCRETRO_AUTOSTART_H
#define PCRETRO_AUTOSTART_H

#include <stdbool.h>

bool pcretro_autostart_enable(void);
bool pcretro_autostart_disable(void);
bool pcretro_autostart_is_enabled(void);

#endif
