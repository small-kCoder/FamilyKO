/* 密码：PBKDF2-SHA256（通过 Windows BCrypt API） */
#include "password.h"
#include "../storage/repo.h"
#include <bcrypt.h>
#include <wctype.h>

#pragma comment(lib, "bcrypt.lib")

#define ITERATIONS 100000
#define SALT_LEN 16
#define HASH_LEN 32

static bool derive_pbkdf2(const wchar_t* password, const unsigned char* salt, int salt_len,
    unsigned char* out, int out_len)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS st = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(st)) return false;
    char* utf8 = pcretro_wide_to_utf8(password);
    int pwd_len = utf8 ? (int)strlen(utf8) : 0;
    st = BCryptDeriveKeyPBKDF2(hAlg, (PUCHAR)utf8, pwd_len, (PUCHAR)salt, salt_len,
        ITERATIONS, out, out_len, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    if (utf8) free(utf8);
    return BCRYPT_SUCCESS(st);
}

bool pcretro_password_is_strong(const wchar_t* password) {
    if (!password) return false;
    int n = (int)wcslen(password);
    if (n < 8) return false;
    bool has_alpha = false, has_digit = false;
    for (int i = 0; i < n; i++) {
        if (iswalpha(password[i])) has_alpha = true;
        if (iswdigit(password[i])) has_digit = true;
    }
    return has_alpha && has_digit;
}

static char* base64_encode(const unsigned char* in, int len) {
    static const char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int olen = 4 * ((len + 2) / 3);
    char* out = (char*)calloc((size_t)olen + 1, 1);
    int j = 0;
    for (int i = 0; i < len; i += 3) {
        int v = in[i] << 16;
        if (i + 1 < len) v |= in[i+1] << 8;
        if (i + 2 < len) v |= in[i+2];
        out[j++] = tab[(v >> 18) & 0x3F];
        out[j++] = tab[(v >> 12) & 0x3F];
        out[j++] = (i + 1 < len) ? tab[(v >> 6) & 0x3F] : '=';
        out[j++] = (i + 2 < len) ? tab[v & 0x3F] : '=';
    }
    out[j] = 0;
    return out;
}

static int base64_decode(const char* in, unsigned char* out) {
    static int tab[256] = {0};
    static bool inited = false;
    if (!inited) {
        const char* alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 256; i++) tab[i] = -1;
        for (int i = 0; i < 64; i++) tab[(unsigned char)alpha[i]] = i;
        inited = true;
    }
    int len = (int)strlen(in);
    int j = 0;
    for (int i = 0; i < len; i += 4) {
        int v = 0, bits = 0;
        for (int k = 0; k < 4 && i + k < len; k++) {
            int c = tab[(unsigned char)in[i+k]];
            if (c < 0) break;
            v = (v << 6) | c;
            bits += 6;
        }
        if (bits >= 8) out[j++] = (v >> (bits - 8)) & 0xFF;
        if (bits >= 16) out[j++] = (v >> (bits - 16)) & 0xFF;
        if (bits >= 18) out[j++] = (v >> (bits - 24)) & 0xFF;
    }
    return j;
}

int pcretro_password_set(const wchar_t* password) {
    if (!pcretro_password_is_strong(password)) return -1;
    unsigned char salt[SALT_LEN];
    unsigned char hash[HASH_LEN];
    if (!BCRYPT_SUCCESS(BCryptGenRandom(NULL, salt, SALT_LEN, BCRYPT_USE_SYSTEM_PREFERRED_RNG))) return -1;
    if (!derive_pbkdf2(password, salt, SALT_LEN, hash, HASH_LEN)) return -1;
    char* salt_b64 = base64_encode(salt, SALT_LEN);
    char* hash_b64 = base64_encode(hash, HASH_LEN);
    char stored[256];
    _snprintf(stored, sizeof(stored), "pbkdf2$%d$%s$%s", ITERATIONS, salt_b64, hash_b64);
    pcretro_repo_setting_set("password_hash", stored);
    pcretro_repo_setting_set("password_initialized", "1");
    free(salt_b64); free(hash_b64);
    return 0;
}

bool pcretro_password_verify(const wchar_t* password) {
    const char* stored = pcretro_repo_setting_get("password_hash");
    if (!*stored) return false;
    int iters = 0;
    char salt_b64[128] = {0}, hash_b64[128] = {0};
    if (sscanf(stored, "pbkdf2$%d$%127[^$]$%127[^$]", &iters, salt_b64, hash_b64) != 3) return false;
    unsigned char salt[SALT_LEN], hash[HASH_LEN], got[HASH_LEN];
    int sn = base64_decode(salt_b64, salt);
    int hn = base64_decode(hash_b64, hash);
    if (sn != SALT_LEN || hn != HASH_LEN) return false;
    if (!derive_pbkdf2(password, salt, sn, got, HASH_LEN)) return false;
    unsigned char diff = 0;
    for (int i = 0; i < HASH_LEN; i++) diff |= got[i] ^ hash[i];
    return diff == 0;
}

bool pcretro_password_is_initialized(void) {
    const char* s = pcretro_repo_setting_get("password_initialized");
    if (strcmp(s, "1") != 0) return false;
    const char* h = pcretro_repo_setting_get("password_hash");
    return *h != 0;
}
