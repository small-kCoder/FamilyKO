/* ACL：拒绝 Users 组 PROCESS_TERMINATE */
#include "acl.h"
#include "../common.h"
#include <windows.h>
#include <sddl.h>

bool pcretro_acl_protect_self(void) {
    if (!pcretro_is_admin()) {
        pcretro_info(L"非管理员运行，跳过 ACL 保护");
        return false;
    }
    /* 构建 SD：允许当前用户/管理员/SYSTEM 全部；拒绝 Users 终止 */
    /* 使用 SDDL 字符串：Deny Users terminate; Allow All */
    const wchar_t* sddl = L"D:(D;;0x0001;;;BU)(A;;0x1FFFFF;;;BA)(A;;0x1FFFFF;;;SY)(A;;0x1FFFFF;;;WD)";
    PSECURITY_DESCRIPTOR pSD = NULL;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl, SDDL_REVISION_1, &pSD, NULL)) {
        pcretro_warn(L"ConvertStringSecurityDescriptorToSecurityDescriptorW 失败: %d", GetLastError());
        return false;
    }
    HANDLE hProc = OpenProcess(0x0600, FALSE, GetCurrentProcessId());
    if (!hProc) { LocalFree(pSD); return false; }
    /* 应用 DACL */
    PACL pDacl = NULL;
    BOOL daclPresent = FALSE, daclDefaulted = FALSE;
    if (!GetSecurityDescriptorDacl(pSD, &daclPresent, &pDacl, &daclDefaulted) || !pDacl) {
        CloseHandle(hProc); LocalFree(pSD); return false;
    }
    NTSTATUS st = SetSecurityInfo(hProc, SE_KERNEL_OBJECT,
        DACL_SECURITY_INFORMATION | PROCESS_SECURITY_INFORMATION,
        NULL, NULL, pDacl, NULL);
    CloseHandle(hProc);
    LocalFree(pSD);
    if (st != 0) {
        pcretro_warn(L"SetSecurityInfo 失败: %d", st);
        return false;
    }
    pcretro_info(L"ACL 保护已应用");
    return true;
}
