/* AI 总结器 */
#ifndef PCRETRO_SUMMARIZER_H
#define PCRETRO_SUMMARIZER_H

#include "../common.h"

#define MAX_AI_EVENTS 200

typedef struct {
    char content[256];
    void (*on_chunk)(const wchar_t* chunk, void* ud);
    void* ud;
} pcretro_summarizer_ctx_t;

/* mode: "ai" or "stats" */
int pcretro_summarizer_run(pcretro_range_t* tr, int child_id, const wchar_t* child_name,
    pcretro_summarizer_ctx_t* ctx, char* out_mode, wchar_t* out_text, int out_text_cap);

#endif
