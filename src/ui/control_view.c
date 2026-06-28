/* UI: 家长控制视图（黑名单 + 限时） */
#include "ui/control_view.h"
#include "ui/dialogs.h"
#include "common.h"
#include "storage/repo.h"
#include "control/blacklist.h"
#include "control/time_limits.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#define IDC_BL_LIST 7001
#define IDC_BL_ADD  7002
#define IDC_BL_DEL  7003
#define IDC_BL_TOGGLE 7004
#define IDC_TL_EDIT 7005

static HWND g_bl_list = NULL;
static HWND g_tl_label = NULL;
static HWND g_root = NULL;

static void refresh_bl(void) {
    ListView_DeleteAllItems(g_bl_list);
    pcretro_blacklist_t* arr = NULL; int n = 0;
    pcretro_repo_blacklist_list(0, &arr, &n);
    LVITEMW li = {0};
    li.mask = LVIF_TEXT;
    for (int i = 0; i < n; i++) {
        wchar_t* pn = pcretro_utf8_to_wide(arr[i].process_name);
        wchar_t* dn = pcretro_utf8_to_wide(arr[i].display_name);
        wchar_t en[8];
        _snwprintf(en, 8, L"%s", arr[i].enabled ? L"是" : L"否");
        li.iItem = i; li.pszText = pn;  ListView_InsertItem(g_bl_list, &li);
        li.iSubItem = 1; li.pszText = dn; ListView_SetItem(g_bl_list, &li);
        li.iSubItem = 2; li.pszText = en; ListView_SetItem(g_bl_list, &li);
        free(pn); free(dn);
    }
    pcretro_repo_blacklist_free(arr);
}

static void refresh_tl(void) {
    int d = 0, m = 0, sm = 0, em = 0, en = 0;
    pcretro_repo_time_limits_get(0, &d, &m, &sm, &em, &en);
    wchar_t buf[256];
    if (!en) {
        wcscpy(buf, L"当前未设置每日使用时长限制。");
    } else {
        _snwprintf(buf, 256,
            L"每日限制：%d 分钟  适用星期掩码：%d  时间窗口：%02d:%02d - %02d:%02d",
            d, m, sm / 60, sm % 60, em / 60, em % 60);
    }
    SetWindowTextW(g_tl_label, buf);
}

static LRESULT CALLBACK root_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)l;
    if (msg == WM_COMMAND) {
        int id = LOWORD(w);
        if (id == IDC_BL_ADD) {
            wchar_t name[128] = {0};
            if (pcretro_dlg_add_blacklist(hwnd, name, 128)) {
                char* n8 = pcretro_wide_to_utf8(name);
                pcretro_repo_blacklist_add(n8, n8);
                if (n8) free(n8);
                refresh_bl();
            }
        } else if (id == IDC_BL_DEL) {
            int sel = ListView_GetNextItem(g_bl_list, -1, LVNI_SELECTED);
            if (sel >= 0) {
                wchar_t buf[128] = {0};
                ListView_GetItemText(g_bl_list, sel, 0, buf, 128);
                char* n8 = pcretro_wide_to_utf8(buf);
                pcretro_repo_blacklist_remove(n8);
                if (n8) free(n8);
                refresh_bl();
            }
        } else if (id == IDC_BL_TOGGLE) {
            MessageBoxW(hwnd, L"请使用 SQL 工具切换 enabled 字段。", L"PCRetro", MB_ICONINFORMATION);
        } else if (id == IDC_TL_EDIT) {
            if (pcretro_dlg_edit_time_limit(hwnd, NULL)) refresh_tl();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

HWND pcretro_control_view_create(HWND parent) {
    g_root = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 100, 100, parent, NULL, GetModuleHandleW(NULL), NULL);
    SetWindowLongPtrW(g_root, GWLP_WNDPROC, (LONG_PTR)root_proc);

    CreateWindowExW(0, L"STATIC", L"应用黑名单：", WS_CHILD | WS_VISIBLE,
        10, 10, 200, 22, g_root, NULL, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"添加", WS_CHILD | WS_VISIBLE,
        600, 10, 60, 26, g_root, (HMENU)IDC_BL_ADD, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"删除", WS_CHILD | WS_VISIBLE,
        670, 10, 60, 26, g_root, (HMENU)IDC_BL_DEL, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"启用/停用", WS_CHILD | WS_VISIBLE,
        740, 10, 80, 26, g_root, (HMENU)IDC_BL_TOGGLE, NULL, NULL);

    g_bl_list = CreateWindowExW(0, WC_LISTVIEWW, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 40, 800, 240, g_root, (HMENU)IDC_BL_LIST, NULL, NULL);
    ListView_SetExtendedListViewStyle(g_bl_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 200; col.pszText = L"进程名";   ListView_InsertColumn(g_bl_list, 0, &col);
    col.cx = 400; col.pszText = L"显示名称"; ListView_InsertColumn(g_bl_list, 1, &col);
    col.cx = 100; col.pszText = L"启用";     ListView_InsertColumn(g_bl_list, 2, &col);

    CreateWindowExW(0, L"STATIC", L"每日使用时长限制：", WS_CHILD | WS_VISIBLE,
        10, 300, 200, 22, g_root, NULL, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"编辑", WS_CHILD | WS_VISIBLE,
        700, 300, 60, 26, g_root, (HMENU)IDC_TL_EDIT, NULL, NULL);
    g_tl_label = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        10, 330, 800, 22, g_root, NULL, NULL, NULL);

    refresh_bl();
    refresh_tl();
    return g_root;
}

void pcretro_control_view_refresh(HWND self) {
    (void)self;
    refresh_bl();
    refresh_tl();
}
