/* UI: 统计视图（按应用/小时聚合） */
#include "ui/stats_view.h"
#include "common.h"
#include "storage/repo.h"
#include "query/time_ranges.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#define IDC_LIST 5001
#define IDC_RANGE 5002

static HWND g_list = NULL;
static HWND g_combo = NULL;
static HWND g_root = NULL;

static void fill(const wchar_t* range) {
    if (!g_list) return;
    ListView_DeleteAllItems(g_list);
    pcretro_range_t r;
    if (!wcscmp(range, L"今天")) pcretro_range_today(&r);
    else if (!wcscmp(range, L"近 7 天")) pcretro_range_last_7_days(&r);
    else pcretro_range_today(&r);

    pcretro_stat_t* arr = NULL; int n = 0;
    pcretro_repo_stats_by_app(r.start_ts, r.end_ts, 0, &arr, &n);

    LVITEMW li = {0};
    li.mask = LVIF_TEXT;
    for (int i = 0; i < n; i++) {
        wchar_t* src = pcretro_utf8_to_wide(arr[i].source);
        wchar_t opens[16], dur[32];
        _snwprintf(opens, 16, L"%d", arr[i].opens);
        _snwprintf(dur, 32, L"%lld 分 %lld 秒",
            arr[i].total_ms / 60000, (arr[i].total_ms / 1000) % 60);
        li.iItem = i; li.pszText = src;    ListView_InsertItem(g_list, &li);
        li.iSubItem = 1; li.pszText = opens; ListView_SetItem(g_list, &li);
        li.iSubItem = 2; li.pszText = dur;   ListView_SetItem(g_list, &li);
        free(src);
    }
    pcretro_repo_free_stats(arr);
}

static LRESULT CALLBACK root_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)l;
    if (msg == WM_COMMAND && LOWORD(w) == IDC_RANGE && HIWORD(w) == CBN_SELCHANGE) {
        wchar_t range[32] = {0};
        int sel = (int)SendMessageW(g_combo, CB_GETCURSEL, 0, 0);
        SendMessageW(g_combo, CB_GETLBTEXT, sel, (LPARAM)range);
        fill(range);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

HWND pcretro_stats_view_create(HWND parent) {
    g_root = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 100, 100, parent, NULL, GetModuleHandleW(NULL), NULL);
    SetWindowLongPtrW(g_root, GWLP_WNDPROC, (LONG_PTR)root_proc);

    CreateWindowExW(0, L"STATIC", L"时间范围：", WS_CHILD | WS_VISIBLE,
        10, 10, 70, 22, g_root, NULL, NULL, NULL);
    g_combo = CreateWindowExW(0, L"COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        80, 10, 120, 100, g_root, (HMENU)IDC_RANGE, NULL, NULL);
    const wchar_t* items[] = { L"今天", L"近 7 天" };
    for (int i = 0; i < 2; i++) SendMessageW(g_combo, CB_ADDSTRING, 0, (LPARAM)items[i]);
    SendMessageW(g_combo, CB_SETCURSEL, 0, 0);

    g_list = CreateWindowExW(0, WC_LISTVIEWW, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT,
        10, 50, 800, 400, g_root, (HMENU)IDC_LIST, NULL, NULL);
    ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 360; col.pszText = L"应用";   ListView_InsertColumn(g_list, 0, &col);
    col.cx = 100; col.pszText = L"打开次数"; ListView_InsertColumn(g_list, 1, &col);
    col.cx = 160; col.pszText = L"总时长";   ListView_InsertColumn(g_list, 2, &col);
    fill(L"今天");
    return g_root;
}

void pcretro_stats_view_refresh(HWND self, const wchar_t* range) {
    (void)self;
    if (range) {
        int sel = 0;
        if (!wcscmp(range, L"近 7 天")) sel = 1;
        SendMessageW(g_combo, CB_SETCURSEL, sel, 0);
    }
    fill(range);
}
