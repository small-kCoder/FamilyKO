/* 仓库层实现 */
#include "repo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static void copy_text(char* dst, size_t cap, const unsigned char* src) {
    if (!src) { dst[0] = 0; return; }
    size_t n = strlen((const char*)src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
}

int64_t pcretro_repo_insert_event(
    const char* kind, const char* source, const char* title,
    const char* payload_json, int duration_ms, int child_id)
{
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    if (child_id <= 0) child_id = pcretro_repo_setting_get_int("active_child_id", 1);
    int64_t ts = pcretro_now();
    sqlite3_stmt* st = NULL;
    const char* sql = "INSERT INTO events(ts, kind, source, title, payload, duration_ms, child_id) "
                      "VALUES(?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db->handle, sql, -1, &st, NULL) != SQLITE_OK) {
        pcretro_error(L"prepare insert_event 失败");
        return -1;
    }
    sqlite3_bind_int64(st, 1, ts);
    sqlite3_bind_text(st, 2, kind ? kind : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, source ? source : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, title ? title : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 5, payload_json ? payload_json : "{}", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 6, duration_ms);
    sqlite3_bind_int(st, 7, child_id);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) {
        pcretro_error(L"insert_event step 失败: %d", rc);
        return -1;
    }
    return sqlite3_last_insert_rowid(db->handle);
}

int pcretro_repo_insert_events_batch(const char* kind,
    const char* const* sources, const char* const* titles,
    const char* const* payloads, const int* durations, int n)
{
    pcretro_db_t* db = pcretro_db_thread();
    if (!db || n <= 0) return 0;
    if (sqlite3_exec(db->handle, "BEGIN", NULL, NULL, NULL) != SQLITE_OK) return 0;
    int count = 0;
    for (int i = 0; i < n; i++) {
        int64_t id = pcretro_repo_insert_event(
            kind, sources[i], titles[i],
            payloads ? payloads[i] : NULL, durations ? durations[i] : 0, 0);
        if (id > 0) count++;
    }
    sqlite3_exec(db->handle, "COMMIT", NULL, NULL, NULL);
    return count;
}

static void bind_like_optional(sqlite3_stmt* st, int idx, const char* s) {
    if (s && *s) {
        char buf[1024];
        _snprintf(buf, sizeof(buf), "%%%s%%", s);
        sqlite3_bind_text(st, idx, buf, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(st, idx);
    }
}

int pcretro_repo_query_events(
    int64_t start_ts, int64_t end_ts,
    const char* const* kinds, int n_kinds,
    const char* keyword, int child_id, int limit,
    pcretro_event_t** out, int* out_count)
{
    pcretro_db_t* db = pcretro_db_thread();
    *out = NULL; *out_count = 0;
    if (!db) return -1;

    char sql[2048];
    int pos = _snprintf(sql, sizeof(sql),
        "SELECT id, ts, kind, source, title, payload, duration_ms, child_id "
        "FROM events WHERE ts BETWEEN ? AND ?");
    if (n_kinds > 0) {
        pos += _snprintf(sql + pos, sizeof(sql) - pos, " AND kind IN (");
        for (int i = 0; i < n_kinds; i++) {
            pos += _snprintf(sql + pos, sizeof(sql) - pos, "%s?", i == 0 ? "" : ",");
        }
        pos += _snprintf(sql + pos, sizeof(sql) - pos, ")");
    }
    if (child_id > 0) {
        pos += _snprintf(sql + pos, sizeof(sql) - pos, " AND child_id=?");
    }
    if (keyword && *keyword) {
        pos += _snprintf(sql + pos, sizeof(sql) - pos, " AND (source LIKE ? OR title LIKE ?)");
    }
    pos += _snprintf(sql + pos, sizeof(sql) - pos, " ORDER BY ts DESC LIMIT ?");

    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &st, NULL) != SQLITE_OK) {
        pcretro_error(L"prepare query_events 失败: %hs", sqlite3_errmsg(db->handle));
        return -1;
    }
    int idx = 1;
    sqlite3_bind_int64(st, idx++, start_ts);
    sqlite3_bind_int64(st, idx++, end_ts);
    for (int i = 0; i < n_kinds; i++) sqlite3_bind_text(st, idx++, kinds[i], -1, SQLITE_TRANSIENT);
    if (child_id > 0) sqlite3_bind_int(st, idx++, child_id);
    if (keyword && *keyword) {
        bind_like_optional(st, idx++, keyword);
        bind_like_optional(st, idx++, keyword);
    }
    sqlite3_bind_int(st, idx++, limit > 0 ? limit : 1000);

    int cap = 0, n = 0;
    pcretro_event_t* arr = NULL;
    while (sqlite3_step(st) == SQLITE_ROW) {
        if (n >= cap) {
            cap = cap ? cap * 2 : 64;
            arr = (pcretro_event_t*)realloc(arr, cap * sizeof(pcretro_event_t));
        }
        pcretro_event_t* e = &arr[n++];
        e->id = sqlite3_column_int64(st, 0);
        e->ts = sqlite3_column_int64(st, 1);
        copy_text(e->kind, sizeof(e->kind), sqlite3_column_text(st, 2));
        copy_text(e->source, sizeof(e->source), sqlite3_column_text(st, 3));
        copy_text(e->title, sizeof(e->title), sqlite3_column_text(st, 4));
        copy_text(e->payload, sizeof(e->payload), sqlite3_column_text(st, 5));
        e->duration_ms = sqlite3_column_int64(st, 6);
        e->child_id = sqlite3_column_int(st, 7);
    }
    sqlite3_finalize(st);
    *out = arr; *out_count = n;
    return 0;
}

void pcretro_repo_free_events(pcretro_event_t* arr) { free(arr); }

int pcretro_repo_stats_by_app(int64_t start_ts, int64_t end_ts, int child_id,
    pcretro_stat_t** out, int* out_count)
{
    pcretro_db_t* db = pcretro_db_thread();
    *out = NULL; *out_count = 0;
    if (!db) return -1;

    const char* sql = "SELECT source, COUNT(*), COALESCE(SUM(duration_ms),0), "
                      "COALESCE(MIN(ts),0), COALESCE(MAX(ts),0) "
                      "FROM events WHERE kind='app' AND ts BETWEEN ? AND ?"
                      + 0; /* no-op */
    char full[1024];
    if (child_id > 0) {
        _snprintf(full, sizeof(full),
            "SELECT source, COUNT(*), COALESCE(SUM(duration_ms),0), COALESCE(MIN(ts),0), COALESCE(MAX(ts),0) "
            "FROM events WHERE kind='app' AND ts BETWEEN ? AND ? AND child_id=? "
            "GROUP BY source ORDER BY 3 DESC");
    } else {
        _snprintf(full, sizeof(full),
            "SELECT source, COUNT(*), COALESCE(SUM(duration_ms),0), COALESCE(MIN(ts),0), COALESCE(MAX(ts),0) "
            "FROM events WHERE kind='app' AND ts BETWEEN ? AND ? "
            "GROUP BY source ORDER BY 3 DESC");
    }
    (void)sql; /* silence unused */

    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, full, -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int64(st, 1, start_ts);
    sqlite3_bind_int64(st, 2, end_ts);
    if (child_id > 0) sqlite3_bind_int(st, 3, child_id);

    int cap = 0, n = 0;
    pcretro_stat_t* arr = NULL;
    while (sqlite3_step(st) == SQLITE_ROW) {
        if (n >= cap) {
            cap = cap ? cap * 2 : 32;
            arr = (pcretro_stat_t*)realloc(arr, cap * sizeof(pcretro_stat_t));
        }
        pcretro_stat_t* s = &arr[n++];
        copy_text(s->source, sizeof(s->source), sqlite3_column_text(st, 0));
        s->opens = sqlite3_column_int(st, 1);
        s->total_ms = sqlite3_column_int64(st, 2);
        s->first_ts = sqlite3_column_int64(st, 3);
        s->last_ts = sqlite3_column_int64(st, 4);
    }
    sqlite3_finalize(st);
    *out = arr; *out_count = n;
    return 0;
}

void pcretro_repo_free_stats(pcretro_stat_t* arr) { free(arr); }

int pcretro_repo_blacklist_add(const char* process_name, const char* display_name) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    sqlite3_stmt* st = NULL;
    const char* sql = "INSERT OR IGNORE INTO app_blacklist(process_name, display_name, added_at, enabled) VALUES(?, ?, ?, 1)";
    if (sqlite3_prepare_v2(db->handle, sql, -1, &st, NULL) != SQLITE_OK) return -1;
    /* 转小写 */
    char name[256];
    _snprintf(name, sizeof(name), "%s", process_name ? process_name : "");
    for (char* p = name; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;
    sqlite3_bind_text(st, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, display_name ? display_name : process_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 3, pcretro_now());
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

int pcretro_repo_blacklist_remove(const char* process_name) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    char name[256];
    _snprintf(name, sizeof(name), "%s", process_name ? process_name : "");
    for (char* p = name; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "DELETE FROM app_blacklist WHERE process_name=?", -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(st, 1, name, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

int pcretro_repo_blacklist_list(int enabled_only, pcretro_blacklist_t** out, int* out_count) {
    pcretro_db_t* db = pcretro_db_thread();
    *out = NULL; *out_count = 0;
    if (!db) return -1;
    sqlite3_stmt* st = NULL;
    const char* sql = enabled_only
        ? "SELECT id, process_name, display_name, added_at, enabled FROM app_blacklist WHERE enabled=1 ORDER BY added_at DESC"
        : "SELECT id, process_name, display_name, added_at, enabled FROM app_blacklist ORDER BY added_at DESC";
    if (sqlite3_prepare_v2(db->handle, sql, -1, &st, NULL) != SQLITE_OK) return -1;
    int cap = 0, n = 0;
    pcretro_blacklist_t* arr = NULL;
    while (sqlite3_step(st) == SQLITE_ROW) {
        if (n >= cap) {
            cap = cap ? cap * 2 : 16;
            arr = (pcretro_blacklist_t*)realloc(arr, cap * sizeof(pcretro_blacklist_t));
        }
        pcretro_blacklist_t* b = &arr[n++];
        b->id = sqlite3_column_int(st, 0);
        copy_text(b->process_name, sizeof(b->process_name), sqlite3_column_text(st, 1));
        copy_text(b->display_name, sizeof(b->display_name), sqlite3_column_text(st, 2));
        b->added_at = sqlite3_column_int64(st, 3);
        b->enabled = sqlite3_column_int(st, 4);
    }
    sqlite3_finalize(st);
    *out = arr; *out_count = n;
    return 0;
}

void pcretro_repo_blacklist_free(pcretro_blacklist_t* arr) { free(arr); }

int pcretro_repo_time_limits_upsert(int child_id, int daily_limit_minutes,
    int weekday_mask, int start_minute, int end_minute, int enabled)
{
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    sqlite3_exec(db->handle, "BEGIN", NULL, NULL, NULL);
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "DELETE FROM time_limits WHERE child_id IS ?", -1, &st, NULL) == SQLITE_OK) {
        sqlite3_bind_int(st, 1, child_id > 0 ? child_id : 0);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }
    if (sqlite3_prepare_v2(db->handle,
        "INSERT INTO time_limits(child_id, daily_limit_minutes, weekday_mask, start_minute_of_day, end_minute_of_day, enabled) "
        "VALUES(?, ?, ?, ?, ?, ?)", -1, &st, NULL) == SQLITE_OK) {
        sqlite3_bind_int(st, 1, child_id);
        sqlite3_bind_int(st, 2, daily_limit_minutes);
        sqlite3_bind_int(st, 3, weekday_mask);
        sqlite3_bind_int(st, 4, start_minute);
        sqlite3_bind_int(st, 5, end_minute);
        sqlite3_bind_int(st, 6, enabled ? 1 : 0);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }
    sqlite3_exec(db->handle, "COMMIT", NULL, NULL, NULL);
    return 0;
}

int pcretro_repo_time_limits_get(int child_id,
    int* daily_limit_minutes, int* weekday_mask, int* start_minute, int* end_minute, int* enabled)
{
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle,
        "SELECT daily_limit_minutes, weekday_mask, start_minute_of_day, end_minute_of_day, enabled "
        "FROM time_limits WHERE child_id IS ? OR (child_id IS NULL AND ? = 0) LIMIT 1",
        -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(st, 1, child_id > 0 ? child_id : 0);
    sqlite3_bind_int(st, 2, child_id);
    int rc = sqlite3_step(st);
    if (rc != SQLITE_ROW) { sqlite3_finalize(st); return -1; }
    *daily_limit_minutes = sqlite3_column_int(st, 0);
    *weekday_mask = sqlite3_column_int(st, 1);
    *start_minute = sqlite3_column_int(st, 2);
    *end_minute = sqlite3_column_int(st, 3);
    *enabled = sqlite3_column_int(st, 4);
    sqlite3_finalize(st);
    return 0;
}

const char* pcretro_repo_setting_get(const char* key) {
    pcretro_db_t* db = pcretro_db_thread();
    static char buf[8192];
    buf[0] = 0;
    if (!db) return buf;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "SELECT value FROM settings WHERE key=?", -1, &st, NULL) != SQLITE_OK) return buf;
    sqlite3_bind_text(st, 1, key, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* v = sqlite3_column_text(st, 0);
        if (v) {
            size_t n = strlen((const char*)v);
            if (n >= sizeof(buf)) n = sizeof(buf) - 1;
            memcpy(buf, v, n);
            buf[n] = 0;
        }
    }
    sqlite3_finalize(st);
    return buf;
}

int pcretro_repo_setting_set(const char* key, const char* value) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle,
        "INSERT INTO settings(key, value) VALUES(?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value", -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(st, 1, key, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, value ? value : "", -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

int pcretro_repo_setting_set_int(const char* key, int v) {
    char buf[32];
    _snprintf(buf, sizeof(buf), "%d", v);
    return pcretro_repo_setting_set(key, buf);
}

int pcretro_repo_setting_get_int(const char* key, int defv) {
    const char* s = pcretro_repo_setting_get(key);
    if (!*s) return defv;
    return atoi(s);
}

void pcretro_repo_lockout_get(int* failures, int64_t* locked_until) {
    pcretro_db_t* db = pcretro_db_thread();
    *failures = 0; *locked_until = 0;
    if (!db) return;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "SELECT failures, locked_until FROM lockout WHERE id=1", -1, &st, NULL) != SQLITE_OK) return;
    if (sqlite3_step(st) == SQLITE_ROW) {
        *failures = sqlite3_column_int(st, 0);
        *locked_until = sqlite3_column_int64(st, 1);
    }
    sqlite3_finalize(st);
}

void pcretro_repo_lockout_set(int failures, int64_t locked_until) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle,
        "UPDATE lockout SET failures=?, locked_until=? WHERE id=1", -1, &st, NULL) != SQLITE_OK) return;
    sqlite3_bind_int(st, 1, failures);
    sqlite3_bind_int64(st, 2, locked_until);
    sqlite3_step(st);
    sqlite3_finalize(st);
}

int pcretro_repo_children_list(pcretro_child_t** out, int* out_count) {
    pcretro_db_t* db = pcretro_db_thread();
    *out = NULL; *out_count = 0;
    if (!db) return -1;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "SELECT id, name, created_at FROM children ORDER BY id", -1, &st, NULL) != SQLITE_OK) return -1;
    int cap = 0, n = 0;
    pcretro_child_t* arr = NULL;
    while (sqlite3_step(st) == SQLITE_ROW) {
        if (n >= cap) {
            cap = cap ? cap * 2 : 8;
            arr = (pcretro_child_t*)realloc(arr, cap * sizeof(pcretro_child_t));
        }
        pcretro_child_t* c = &arr[n++];
        c->id = sqlite3_column_int(st, 0);
        copy_text(c->name, sizeof(c->name), sqlite3_column_text(st, 1));
        c->created_at = sqlite3_column_int64(st, 2);
    }
    sqlite3_finalize(st);
    *out = arr; *out_count = n;
    return 0;
}

void pcretro_repo_children_free(pcretro_child_t* arr) { free(arr); }

int pcretro_repo_children_add(const char* name) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "INSERT INTO children(name, created_at) VALUES(?, ?)", -1, &st, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(st, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 2, pcretro_now());
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? (int)sqlite3_last_insert_rowid(db->handle) : -1;
}

int pcretro_repo_clear_events(void) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return 0;
    sqlite3_exec(db->handle, "DELETE FROM events", NULL, NULL, NULL);
    return (int)sqlite3_changes(db->handle);
}

int pcretro_repo_vacuum(void) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return -1;
    return sqlite3_exec(db->handle, "VACUUM", NULL, NULL, NULL);
}
