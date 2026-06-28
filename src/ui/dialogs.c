/* UI: 通用对话框（设置/修改密码、添加黑名单、编辑时间限制） */
#include "ui/dialogs.h"
#include "common.h"
#include "security/password.h"
#include "storage/repo.h"

#include <windows.h>
#include <stdio.h>

bool pcretro_dlg_set_password(HWND parent) {
    if (pcretro_password_is_initialized()) {
        MessageBoxW(parent, L"已存在密码，请使用"修改密码"。", L"PCRetro", MB_ICONINFORMATION);
        return false;
    }
    if (MessageBoxW(parent,
        L"将设置默认演示密码：admin1234\r\n（首次安装时方便测试）\r\n\r\n"
        L"实际部署后请立即通过"修改密码"改为强密码。\r\n\r\n是否继续？",
        L"PCRetro", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
        return false;
    }
    return pcretro_password_set(L"admin1234") == 0;
}

bool pcretro_dlg_change_password(HWND parent) {
    if (!pcretro_password_is_initialized()) {
        MessageBoxW(parent, L"尚未设置密码，无法修改。", L"PCRetro", MB_ICONINFORMATION);
        return false;
    }
    /* 演示实现：直接重置为 admin1234 */
    if (MessageBoxW(parent,
        L"（演示功能）确认将密码重置为 admin1234？\r\n生产代码应弹出原密码验证 + 新密码输入对话框。",
        L"PCRetro", MB_OKCANCEL) != IDOK) {
        return false;
    }
    return pcretro_password_set(L"admin1234") == 0;
}

bool pcretro_dlg_add_blacklist(HWND parent, wchar_t* out_name, size_t out_len) {
    (void)parent; (void)out_name; (void)out_len;
    MessageBoxW(parent,
        L"添加黑名单：演示代码未实现对话框。\r\n"
        L"可直接调用 pcretro_repo_blacklist_add(name, name) 添加。",
        L"PCRetro", MB_ICONINFORMATION);
    return false;
}

bool pcretro_dlg_edit_time_limit(HWND parent, void* limit_inout) {
    (void)parent; (void)limit_inout;
    return false;
}
