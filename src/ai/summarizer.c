/* AI 总结器：调用 Ollama，失败时降级为统计 */
#include "summarizer.h"
#include "ollama.h"
#include "../query/search.h"
#include "../query/time_ranges.h"
#include "../storage/repo.h"
#include <stdio.h>

static const wchar_t* SYSTEM_PROMPT =
    L"你是一位温和、理性的家长助手。根据孩子的电脑使用行为记录，"
    L"生成一段客观、有建设性的总结。用中文，口吻平和、像家长与孩子沟通。"
    L"客观描述事实，结尾给出 1-2 条可执行建议，总长度控制在 200 字以内。";

static void serialize_events(pcretro_event_t* events, int n, wchar_t* out, int cap) {
    int pos = 0;
    pos += _snwprintf(out + pos, cap - pos, L"以下是事件列表：\n");
    for (int i = 0; i < n && i < MAX_AI_EVENTS; i++) {
        pcretro_event_t* e = &events[i];
        wchar_t ts[32];
        pcretro_localtime_str(e->ts, ts, 32);
        wchar_t line[512];
        if (strcmp(e->kind, "web") == 0) {
            wchar_t* title_w = pcretro_utf8_to_wide(e->title);
            _snwprintf(line, 512, L"- [%s] 网页：%s\n", ts, title_w ? title_w : L"");
            free(title_w);
        } else if (strcmp(e->kind, "app") == 0) {
            wchar_t* src_w = pcretro_utf8_to_wide(e->source);
            wchar_t* title_w = pcretro_utf8_to_wide(e->title);
            _snwprintf(line, 512, L"- [%s] 应用：%s 标题：%s 时长：%lldms\n",
                ts, src_w ? src_w : L"", title_w ? title_w : L"", e->duration_ms);
            free(src_w); free(title_w);
        } else {
            wchar_t* src_w = pcretro_utf8_to_wide(e->source);
            _snwprintf(line, 512, L"- [%s] %hs：%s\n", ts, e->kind, src_w ? src_w : L"");
            free(src_w);
        }
        pos += _snwprintf(out + pos, cap - pos, L"%s", line);
        if (pos > cap - 256) break;
    }
}

static bool ollama_cb(const char* chunk, void* ud) {
    pcretro_summarizer_ctx_t* ctx = (pcretro_summarizer_ctx_t*)ud;
    if (ctx->on_chunk) {
        wchar_t* w = pcretro_utf8_to_wide(chunk);
        if (w) { ctx->on_chunk(w, ctx->ud); free(w); }
    }
    return true;
}

int pcretro_summarizer_run(pcretro_range_t* tr, int child_id, const wchar_t* child_name,
    pcretro_summarizer_ctx_t* ctx, char* out_mode, wchar_t* out_text, int out_text_cap)
{
    out_text[0] = 0;
    strcpy(out_mode, "ai");

    if (!pcretro_ollama_is_available()) {
        strcpy(out_mode, "stats");
        pcretro_stat_t* stats = NULL; int n = 0;
        pcretro_search_stats(tr, child_id, &stats, &n);
        _snwprintf(out_text, out_text_cap, L"本地 AI 未启动。以下为纯统计：\n");
        int pos = (int)wcslen(out_text);
        for (int i = 0; i < n && i < 5; i++) {
            wchar_t* src_w = pcretro_utf8_to_wide(stats[i].source);
            int sec = (int)(stats[i].total_ms / 1000);
            int h = sec / 3600, m = (sec % 3600) / 60;
            pos += _snwprintf(out_text + pos, out_text_cap - pos,
                L"  - %s：%d 小时 %d 分（%d 次）\n",
                src_w ? src_w : L"", h, m, stats[i].opens);
            free(src_w);
        }
        pcretro_repo_free_stats(stats);
        return 0;
    }

    pcretro_event_t* events = NULL; int n = 0;
    pcretro_search_events(tr, NULL, 0, NULL, child_id, MAX_AI_EVENTS, &events, &n);
    if (n == 0) {
        wcsncpy(out_text, L"该时间段没有任何记录，无需总结。", out_text_cap - 1);
        pcretro_repo_free_events(events);
        return 0;
    }

    wchar_t events_text[32768];
    serialize_events(events, n, events_text, 32768);
    pcretro_repo_free_events(events);

    wchar_t user[35000];
    _snwprintf(user, 35000,
        L"以下是【%s】%s 的电脑使用记录（已脱敏），请用中文家长视角总结：\n\n%ls\n\n请输出不超过 200 字的总结与建议。",
        child_name, tr->label, events_text);

    if (!pcretro_ollama_chat_stream(NULL, SYSTEM_PROMPT, user, ollama_cb, ctx, 30)) {
        strcpy(out_mode, "stats");
        _snwprintf(out_text, out_text_cap, L"AI 调用失败。");
    }
    return 0;
}
