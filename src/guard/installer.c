/* Windows 服务安装/卸载（使用 SCM API） */
#include "guard/installer.h"
#include "common.h"

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>

/* 内部：将 wchar_t* 服务名转 SC_HANDLE */
static SC_HANDLE open_sc(void) {
    return OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
}

/* 查询服务是否已注册 */
bool pcretro_service_is_installed(void) {
    SC_HANDLE sc = open_sc();
    if (!sc) return false;
    SC_HANDLE svc = OpenServiceW(sc, PCRETRO_SERVICE_W, SERVICE_QUERY_CONFIG);
    bool ok = (svc != NULL);
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(sc);
    return ok;
}

/* 查询服务是否正在运行 */
bool pcretro_service_is_running(void) {
    SC_HANDLE sc = open_sc();
    if (!sc) return false;
    SC_HANDLE svc = OpenServiceW(sc, PCRETRO_SERVICE_W, SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(sc); return false; }
    SERVICE_STATUS ss = {0};
    bool running = false;
    if (QueryServiceStatus(svc, &ss)) {
        running = (ss.dwCurrentState == SERVICE_RUNNING);
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(sc);
    return running;
}

/* 安装服务：
 *   - binPath：可执行文件完整路径（PCRetroWatcher.exe）
 *   - 需要管理员权限
 */
bool pcretro_service_install(void) {
    if (!pcretro_is_admin()) {
        pcretro_warn(L"安装服务需要管理员权限");
        return false;
    }
    SC_HANDLE sc = open_sc();
    if (!sc) {
        pcretro_error(L"无法打开 SCM");
        return false;
    }

    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);   /* 当前 exe 路径 */
    /* 替换为同目录下的 PCRetroWatcher.exe */
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) *(slash + 1) = L'\0';
    wcsncat(path, L"PCRetroWatcher.exe", MAX_PATH - wcslen(path) - 1);

    wchar_t cmd[MAX_PATH + 16] = {0};
    swprintf(cmd, MAX_PATH + 16, L"\"%s\" service", path);

    SC_HANDLE svc = CreateServiceW(
        sc,
        PCRETRO_SERVICE_W,
        L"PCRetro Watcher Service",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        cmd,
        NULL, NULL, NULL, NULL, NULL
    );

    bool ok = (svc != NULL);
    if (!ok) {
        DWORD e = GetLastError();
        if (e == ERROR_SERVICE_EXISTS) {
            pcretro_info(L"服务已存在，将覆盖配置");
            svc = OpenServiceW(sc, PCRETRO_SERVICE_W, SERVICE_ALL_ACCESS);
            if (svc) {
                ok = ChangeServiceConfigW(
                    svc, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
                    SERVICE_ERROR_NORMAL, cmd, NULL, NULL, NULL, NULL, NULL, NULL);
            }
        } else {
            pcretro_error(L"CreateService 失败，错误码=%lu", e);
        }
    } else {
        pcretro_info(L"服务已注册：%s", PCRETRO_SERVICE_W);
    }
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(sc);
    return ok;
}

/* 卸载服务 */
bool pcretro_service_uninstall(void) {
    if (!pcretro_service_is_installed()) {
        pcretro_info(L"服务未注册");
        return true;
    }
    if (pcretro_service_is_running()) {
        pcretro_service_stop();
    }
    SC_HANDLE sc = open_sc();
    if (!sc) return false;
    SC_HANDLE svc = OpenServiceW(sc, PCRETRO_SERVICE_W, SERVICE_ALL_ACCESS);
    if (!svc) { CloseServiceHandle(sc); return false; }
    bool ok = DeleteService(svc);
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(sc);
    if (ok) pcretro_info(L"服务已卸载");
    return ok;
}

/* 启动服务 */
bool pcretro_service_start(void) {
    if (!pcretro_service_is_installed()) return false;
    SC_HANDLE sc = open_sc();
    if (!sc) return false;
    SC_HANDLE svc = OpenServiceW(sc, PCRETRO_SERVICE_W, SERVICE_START);
    if (!svc) { CloseServiceHandle(sc); return false; }
    bool ok = StartServiceW(svc, 0, NULL);
    if (!ok && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) ok = true;
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(sc);
    return ok;
}

/* 停止服务 */
bool pcretro_service_stop(void) {
    if (!pcretro_service_is_installed()) return true;
    SC_HANDLE sc = open_sc();
    if (!sc) return false;
    SC_HANDLE svc = OpenServiceW(sc, PCRETRO_SERVICE_W, SERVICE_STOP);
    if (!svc) { CloseServiceHandle(sc); return false; }
    SERVICE_STATUS ss = {0};
    bool ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss);
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(sc);
    return ok;
}
