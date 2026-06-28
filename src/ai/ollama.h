/* Ollama HTTP 客户端（WinHTTP） */
#ifndef PCRETRO_OLLAMA_H
#define PCRETRO_OLLAMA_H

#include "../common.h"

bool pcretro_ollama_is_available(void);
int pcretro_ollama_list_models(char* buf, int buflen);
/* 流式调用：每收到一段 token 调用 cb，返回 false 中止 */
bool pcretro_ollama_chat_stream(const wchar_t* model, const wchar_t* system, const wchar_t* user,
    bool (*cb)(const char* chunk, void* ud), void* ud, int timeout_sec);

#endif
