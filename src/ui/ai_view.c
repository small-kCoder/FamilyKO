/* UI: AI 总结视图 */
#include "ui/ai_view.h"
#include "common.h"
#include "ai/summarizer.h"
#include "query/time_ranges.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#define IDC_EDIT 6001
#define IDC_BTN  6002

static HWND g_edit = NULL;
static HWND g_root = NULL;

static void do_generate(const wchar_t* range) {
    pcretro_range_t r;
    if (!wcscmp(range, L"今天")) pcretro_range_today(&r);
    else if (!wcscmp(range, L"近 7 天")) pcretro_range_last_7_days(&r);
    else pcretro_range_today(&r);

    SetWindowTextW(g_edit, L"正在生成总结…");
    UpdateWindow(g_edit);

    char mode[8] = {0};
    wchar_t text[8192] = {0};
    pcretro_summarizer_ctx_t ctx = {0};
    int rc = pcretro_summarizer_run(&r, 0, L"孩子", &ctx, mode, text, 8192);
    (void)rc;
    if (*text) {
        SetWindowTextW(g_edit, text);
    } else {
        SetWindowTextW(g_edit, L"AI 总结生成失败。请检查 Ollama 是否运行（http://127.0.0.1:11434）。");
    }
}

static LRESULT CALLBACK root_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    (void)l;
    if (msg == WM_COMMAND && LOWORD(w) == IDC_BTN) {
        do_generate(L"今天");
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

HWND pcretro_ai_view_create(HWND parent) {
    g_root = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 100, 100, parent, NULL, GetModuleHandleW(NULL), NULL);
    SetWindowLongPtrW(g_root, GWLP_WNDPROC, (LONG_PTR)root_proc);

    CreateWindowExW(0, L"BUTTON", L"重新生成总结",
        WS_CHILD | WS_VISIBLE,
        10, 10, 160, 28, g_root, (HMENU)IDC_BTN, NULL, NULL);

    g_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"（点击"重新生成总结"按钮生成 AI 总结）",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
        10, 50, 800, 400, g_root, (HMENU)IDC_EDIT, NULL, NULL);

    return g_root;
}

void pcretro_ai_view_refresh(HWND self, const wchar_t* range) {
    (void)self;
    do_generate(range ? range : L"今天");
}

void pcretro_ai_view_cancel(HWND self) {
    (void)self;
}
