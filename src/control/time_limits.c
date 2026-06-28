#include "time_limits.h"
#include "../storage/repo.h"
#include <time.h>

static int daily_usage_sec(int child_id) {
    pcretro_db_t* db = pcretro_db_thread();
    if (!db) return 0;
    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    int64_t start = (int64_t)mktime(&t);
    sqlite3_stmt* st = NULL;
    int sec = 0;
    const char* sql = child_id > 0
        ? "SELECT COALESCE(SUM(duration_ms),0) FROM events WHERE kind='app' AND ts BETWEEN ? AND ? AND child_id=?"
        : "SELECT COALESCE(SUM(duration_ms),0) FROM events WHERE kind='app' AND ts BETWEEN ? AND ?";
    if (sqlite3_prepare_v2(db->handle, sql, -1, &st, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(st, 1, start);
        sqlite3_bind_int64(st, 2, (int64_t)now);
        if (child_id > 0) sqlite3_bind_int(st, 3, child_id);
        if (sqlite3_step(st) == SQLITE_ROW) sec = (int)(sqlite3_column_int64(st, 0) / 1000);
        sqlite3_finalize(st);
    }
    return sec;
}

void pcretro_time_limits_evaluate(int child_id, pcretro_limit_decision_t* out) {
    memset(out, 0, sizeof(*out));
    out->allowed = true;
    wcscpy(out->reason, L"无限制");
    int d, m, sm, em, en;
    if (pcretro_repo_time_limits_get(child_id, &d, &m, &sm, &em, &en) != 0) return;
    if (!en) { wcscpy(out->reason, L"限制已停用"); return; }

    int used = daily_usage_sec(child_id);
    int limit = d * 60;
    out->daily_used_sec = used;
    out->daily_limit_sec = limit;

    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    int mask_day = (m >> t.tm_wday) & 1;  /* tm_wday: 0=Sun */
    int cur_min = t.tm_hour * 60 + t.tm_min;
    bool in_win = em > sm ? (sm <= cur_min && cur_min < em) : true;

    if (m && !mask_day) { out->allowed = false; wcscpy(out->reason, L"今天不在允许日"); return; }
    if (em > sm && !in_win) { out->allowed = false; wcscpy(out->reason, L"不在允许时段"); return; }
    if (limit > 0 && used >= limit) { out->allowed = false; wcscpy(out->reason, L"已达每日上限"); return; }
    out->allowed = true;
    wcscpy(out->reason, L"允许使用");
}
