/* Ollama HTTP 客户端（WinHTTP） */
#include "ollama.h"
#include "../storage/repo.h"
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

static const wchar_t* get_url(void) {
    static wchar_t buf[256];
    const char* s = pcretro_repo_setting_get("ollama_url");
    char* d = buf;
    /* ASCII -> wide */
    for (int i = 0; i < 255 && s[i]; i++) d[i] = (wchar_t)(unsigned char)s[i];
    d[255] = 0;
    return buf;
}

static const wchar_t* get_model(void) {
    static wchar_t buf[128];
    const char* s = pcretro_repo_setting_get("ollama_model");
    int i = 0;
    for (; i < 127 && s[i]; i++) buf[i] = (wchar_t)(unsigned char)s[i];
    buf[i] = 0;
    return buf;
}

bool pcretro_ollama_is_available(void) {
    const wchar_t* url = get_url();
    /* 简单实现：GET /api/tags */
    HINTERNET hSession = WinHttpOpen(L"PCRetro/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    wchar_t host[128] = {0}, path[256] = {0};
    int is_https = 0;
    const wchar_t* p = url;
    if (wcsncmp(p, L"https://", 8) == 0) { is_https = 1; p += 8; }
    else if (wcsncmp(p, L"http://", 7) == 0) { p += 7; }
    else { WinHttpCloseHandle(hSession); return false; }
    int i = 0;
    while (*p && *p != L'/' && i < 127) host[i++] = *p++;
    while (*p && i < 255) path[i++] = *p++;
    host[i] = 0; path[i] = 0;
    if (!host[0]) { wcscpy(host, L"127.0.0.1"); wcscpy(path, L"/"); }

    /* 解析端口 */
    wchar_t host_only[128] = {0};
    int port = is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    wchar_t* colon = wcschr(host, L':');
    if (colon) {
        *colon = 0;
        port = _wtoi(colon + 1);
        wcscpy(host_only, host);
    } else {
        wcscpy(host_only, host);
    }
    HINTERNET hConn = WinHttpConnect(hSession, host_only, port, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", path[0] ? path : L"/",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    bool ok = false;
    if (hReq) {
        if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) &&
            WinHttpReceiveResponse(hReq, NULL)) {
            ok = true;
        }
        WinHttpCloseHandle(hReq);
    }
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return ok;
}

int pcretro_ollama_list_models(char* buf, int buflen) {
    buf[0] = 0;
    (void)buflen;
    /* 简化：不解析 JSON，只返回可用标志 */
    if (!pcretro_ollama_is_available()) return 0;
    return 0;
}

/* 流式 JSON 简化解析：只取 "content":"..." 的内容 */
static bool parse_content(const char* line, char* out, int cap) {
    const char* p = strstr(line, "\"content\":\"");
    if (!p) return false;
    p += strlen("\"content\":\"");
    int n = 0;
    while (*p && *p != '"' && n < cap - 1) {
        if (*p == '\\' && p[1]) {
            p++;
            if (*p == 'n') out[n++] = '\n';
            else if (*p == 't') out[n++] = '\t';
            else if (*p == 'r') out[n++] = '\r';
            else if (*p == '"') out[n++] = '"';
            else if (*p == '\\') out[n++] = '\\';
            else out[n++] = *p;
            p++;
        } else {
            out[n++] = *p++;
        }
    }
    out[n] = 0;
    return n > 0;
}

bool pcretro_ollama_chat_stream(const wchar_t* model, const wchar_t* system, const wchar_t* user,
    bool (*cb)(const char* chunk, void* ud), void* ud, int timeout_sec)
{
    if (!cb) return false;
    const wchar_t* url = get_url();
    wchar_t host[128] = {0}, path[256] = {0};
    int is_https = 0;
    const wchar_t* p = url;
    if (wcsncmp(p, L"https://", 8) == 0) { is_https = 1; p += 8; }
    else if (wcsncmp(p, L"http://", 7) == 0) { p += 7; }
    else return false;
    int i = 0;
    while (*p && *p != L'/' && i < 127) host[i++] = *p++;
    while (*p && i < 255) path[i++] = *p++;
    host[i] = 0; path[i] = 0;
    wchar_t host_only[128] = {0};
    int port = is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    wchar_t* colon = wcschr(host, L':');
    if (colon) { *colon = 0; port = _wtoi(colon + 1); wcscpy(host_only, host); }
    else { wcscpy(host_only, host); }

    HINTERNET hSession = WinHttpOpen(L"PCRetro/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConn = WinHttpConnect(hSession, host_only, port, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", path[0] ? path : L"/",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession); return false; }

    /* 构造 JSON body（最小转义） */
    const wchar_t* use_model = model && *model ? model : get_model();
    /* 简化：只把 system/user 中的 " 转义 */
    char* sys_a = pcretro_wide_to_utf8(system ? system : L"");
    char* user_a = pcretro_wide_to_utf8(user ? user : L"");
    char* model_a = pcretro_wide_to_utf8(use_model);
    /* 简化：直接拼 */
    char body[65536];
    int pos = 0;
    pos += _snprintf(body + pos, sizeof(body) - pos,
        "{\"model\":\"%s\",\"stream\":true,\"messages\":[", model_a ? model_a : "qwen2.5:7b");
    if (sys_a && *sys_a) {
        pos += _snprintf(body + pos, sizeof(body) - pos,
            "{\"role\":\"system\",\"content\":\"");
        for (char* s = sys_a; *s && pos < (int)sizeof(body) - 64; s++) {
            if (*s == '"' || *s == '\\') body[pos++] = '\\';
            body[pos++] = *s;
        }
        pos += _snprintf(body + pos, sizeof(body) - pos, "\"},");
    }
    pos += _snprintf(body + pos, sizeof(body) - pos,
        "{\"role\":\"user\",\"content\":\"");
    for (char* s = user_a; *s && pos < (int)sizeof(body) - 64; s++) {
        if (*s == '"' || *s == '\\') body[pos++] = '\\';
        body[pos++] = *s;
    }
    pos += _snprintf(body + pos, sizeof(body) - pos, "\"}]}");
    body[pos] = 0;
    free(sys_a); free(user_a); free(model_a);

    wchar_t headers[256];
    _snwprintf(headers, 256, L"Content-Type: application/json\r\n");

    int body_len = (int)strlen(body);
    BOOL ok = WinHttpSendRequest(hReq, headers, (DWORD)wcslen(headers),
        body, body_len, body_len, 0);
    if (!ok || !WinHttpReceiveResponse(hReq, NULL)) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession);
        return false;
    }
    DWORD timeout_ms = timeout_sec * 1000;
    WinHttpSetOption(hReq, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout_ms, sizeof(timeout_ms));

    char buf[8192];
    DWORD got = 0;
    bool aborted = false;
    while (!aborted && WinHttpQueryDataAvailable(hReq, &got) && got > 0) {
        DWORD read = 0;
        if (got > sizeof(buf) - 1) got = sizeof(buf) - 1;
        if (!WinHttpReadData(hReq, buf, got, &read) || read == 0) break;
        buf[read] = 0;
        /* 按行解析 JSON Lines */
        char* line = strtok(buf, "\n");
        while (line) {
            char content[1024];
            if (parse_content(line, content, sizeof(content))) {
                if (!cb(content, ud)) { aborted = true; break; }
            }
            line = strtok(NULL, "\n");
        }
    }
    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return !aborted;
}
