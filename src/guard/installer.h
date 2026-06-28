/* Windows 服务安装/卸载 */
#ifndef PCRETRO_GUARD_INSTALLER_H
#define PCRETRO_GUARD_INSTALLER_H

bool pcretro_service_install(void);
bool pcretro_service_uninstall(void);
bool pcretro_service_start(void);
bool pcretro_service_stop(void);
bool pcretro_service_is_installed(void);
bool pcretro_service_is_running(void);

#endif
