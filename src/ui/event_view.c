/* UI: 事件流视图（ListView） */
#include "ui/event_view.h"
#include "common.h"
#include "storage/repo.h"
#include "query/time_ranges.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#define IDC_LIST  4001
#define IDC_RANGE 4002
#define IDC_KW    4003
#define IDC_BTN   4004

static HWND g_list = NULL;
static HWND g_combo = NULL;
static HWND g_kw = NULL;
static HWND g_root = NULL;

static void fill_list(const wchar_t* range, const wchar_t* keyword) {
    if (!g_list) return;
    ListView_DeleteAllItems(g_list);

    pcretro_range_t r;
    if      (!wcscmp(range, L"今天"))  pcretro_range_today(&r);
    else if (!wcscmp(range, L"昨天"))  pcretro_range_yesterday(&r);
    else if (!wcscmp(range, L"昨晚"))  pcretro_range_last_night(&r);
    else if (!wcscmp(range, L"本周"))  pcretro_range_this_week(&r);
    else if (!wcscmp(range, L"近 7 天")) pcretro_range_last_7_days(&r);
    else                                pcretro_range_today(&r);

    char* k8 = (keyword && *keyword) ? pcretro_wide_to_utf8(keyword) : NULL;
    pcretro_event_t* evs = NULL; int n = 0;
    pcretro_repo_query_events(r.start_ts, r.end_ts, NULL, 0, k8, 0, 500, &evs, &n);
    if (k8) free(k8);

    LVITEMW li = {0};
    li.mask = LVIF_TEXT;
    for (int i = 0; i < n; i++) {
        wchar_t ts[32];
        pcretro_localtime_str(evs[i].ts, ts, 32);

        wchar_t* kind = pcretro_utf8_to_wide(evs[i].kind);
        wchar_t* src  = pcretro_utf8_to_wide(evs[i].source);
        wchar_t* ttl  = pcretro_utf8_to_wide(evs[i].title);

        li.iItem = i;
        li.pszText = ts;           ListView_InsertItem(g_list, &li);
        li.iSubItem = 1; li.pszText = kind; ListView_SetItem(g_list, &li);
        li.iSubItem = 2; li.pszText = src;  ListView_SetItem(g_list, &li);
        li.iSubItem = 3; li.pszText = ttl;  ListView_SetItem(g_list, &li);

        free(kind); free(src); free(ttl);
    }
    pcretro_repo_free_events(evs);
}

static LRESULT CALLBACK root_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)l;
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(w) == IDC_BTN) {
            wchar_t range[32] = {0};
            int sel = (int)SendMessageW(g_combo, CB_GETCURSEL, 0, 0);
            SendMessageW(g_combo, CB_GETLBTEXT, sel, (LPARAM)range);
            wchar_t kw[128] = {0};
            GetWindowTextW(g_kw, kw, 128);
            fill_list(range, kw);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

HWND pcretro_event_view_create(HWND parent) {
    g_root = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 100, 100, parent, NULL, GetModuleHandleW(NULL), NULL);
    SetWindowLongPtrW(g_root, GWLP_WNDPROC, (LONG_PTR)root_proc);

    CreateWindowExW(0, L"STATIC", L"时间范围：", WS_CHILD | WS_VISIBLE,
        10, 10, 70, 22, g_root, NULL, NULL, NULL);
    g_combo = CreateWindowExW(0, L"COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        80, 10, 120, 200, g_root, (HMENU)IDC_RANGE, NULL, NULL);
    const wchar_t* items[] = { L"今天", L"昨天", L"昨晚", L"本周", L"近 7 天" };
    for (int i = 0; i < 5; i++) SendMessageW(g_combo, CB_ADDSTRING, 0, (LPARAM)items[i]);
    SendMessageW(g_combo, CB_SETCURSEL, 0, 0);

    CreateWindowExW(0, L"STATIC", L"关键字：", WS_CHILD | WS_VISIBLE,
        210, 10, 60, 22, g_root, NULL, NULL, NULL);
    g_kw = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        270, 10, 200, 24, g_root, (HMENU)IDC_KW, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"查询",
        WS_CHILD | WS_VISIBLE,
        480, 10, 80, 26, g_root, (HMENU)IDC_BTN, NULL, NULL);

    g_list = CreateWindowExW(0, WC_LISTVIEWW, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 50, 800, 400, g_root, (HMENU)IDC_LIST, NULL, NULL);
    ListView_SetExtendedListViewStyle(g_list,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 160; col.pszText = L"时间";        ListView_InsertColumn(g_list, 0, &col);
    col.cx = 80;  col.pszText = L"类型";        ListView_InsertColumn(g_list, 1, &col);
    col.cx = 200; col.pszText = L"应用 / 来源"; ListView_InsertColumn(g_list, 2, &col);
    col.cx = 360; col.pszText = L"标题";        ListView_InsertColumn(g_list, 3, &col);

    fill_list(L"今天", L"");
    return g_root;
}

void pcretro_event_view_refresh(HWND self, const wchar_t* range) {
    (void)self;
    if (range) {
        int sel = 0;
        if      (!wcscmp(range, L"昨天")) sel = 1;
        else if (!wcscmp(range, L"昨晚")) sel = 2;
        else if (!wcscmp(range, L"本周")) sel = 3;
        else if (!wcscmp(range, L"近 7 天")) sel = 4;
        SendMessageW(g_combo, CB_SETCURSEL, sel, 0);
    }
    SendMessageW(g_root, WM_COMMAND, MAKEWPARAM(IDC_BTN, BN_CLICKED), 0);
}
