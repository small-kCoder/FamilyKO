/* SQLite 实现 */
#include "db.h"
#include "../common.h"
#include <process.h>
#include <stdarg.h>

struct pcretro_db {
    sqlite3* handle;
    sqlite3_stmt* stmt_cache[16];
    int stmt_count;
};

/* TLS：每线程独立连接 */
static __declspec(thread) pcretro_db_t* tls_db = NULL;

int pcretro_db_open(pcretro_db_t** out) {
    pcretro_db_t* db = (pcretro_db_t*)calloc(1, sizeof(pcretro_db_t));
    if (!db) return 1;
    const wchar_t* path = pcretro_db_path();
    char* path_a = pcretro_wide_to_utf8(path);
    int rc = sqlite3_open_v2(path_a, &db->handle,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    free(path_a);
    if (rc != SQLITE_OK) {
        pcretro_error(L"打开数据库失败: %hs", sqlite3_errmsg(db->handle));
        sqlite3_close(db->handle);
        free(db);
        return rc;
    }
    sqlite3_busy_timeout(db->handle, 5000);
    sqlite3_exec(db->handle, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    sqlite3_exec(db->handle, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL);
    sqlite3_exec(db->handle, "PRAGMA foreign_keys=ON", NULL, NULL, NULL);
    *out = db;
    return 0;
}

void pcretro_db_close(pcretro_db_t* db) {
    if (!db) return;
    for (int i = 0; i < db->stmt_count; i++) {
        if (db->stmt_cache[i]) sqlite3_finalize(db->stmt_cache[i]);
    }
    if (db->handle) sqlite3_close(db->handle);
    free(db);
}

int pcretro_db_init_schema(pcretro_db_t* db) {
    static const char* ddl =
        "CREATE TABLE IF NOT EXISTS children ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE,"
        "  created_at INTEGER NOT NULL);"

        "CREATE TABLE IF NOT EXISTS events ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  child_id INTEGER,"
        "  ts INTEGER NOT NULL,"
        "  kind TEXT NOT NULL,"
        "  source TEXT,"
        "  title TEXT,"
        "  payload TEXT,"
        "  duration_ms INTEGER DEFAULT 0);"
        "CREATE INDEX IF NOT EXISTS idx_events_ts ON events(ts DESC);"
        "CREATE INDEX IF NOT EXISTS idx_events_kind ON events(kind);"
        "CREATE INDEX IF NOT EXISTS idx_events_child ON events(child_id, ts DESC);"

        "CREATE TABLE IF NOT EXISTS app_blacklist ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  process_name TEXT NOT NULL UNIQUE,"
        "  display_name TEXT,"
        "  added_at INTEGER NOT NULL,"
        "  enabled INTEGER NOT NULL DEFAULT 1);"

        "CREATE TABLE IF NOT EXISTS time_limits ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  child_id INTEGER,"
        "  daily_limit_minutes INTEGER DEFAULT 0,"
        "  weekday_mask INTEGER DEFAULT 0,"
        "  start_minute_of_day INTEGER DEFAULT 0,"
        "  end_minute_of_day INTEGER DEFAULT 1440,"
        "  enabled INTEGER NOT NULL DEFAULT 1);"

        "CREATE TABLE IF NOT EXISTS settings ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT);"

        "CREATE TABLE IF NOT EXISTS crash_log ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ts INTEGER NOT NULL,"
        "  pid INTEGER,"
        "  exit_code INTEGER,"
        "  restarted INTEGER DEFAULT 0,"
        "  note TEXT);"

        "CREATE TABLE IF NOT EXISTS lockout ("
        "  id INTEGER PRIMARY KEY CHECK (id = 1),"
        "  failures INTEGER NOT NULL DEFAULT 0,"
        "  locked_until INTEGER NOT NULL DEFAULT 0);"
        "INSERT OR IGNORE INTO lockout(id, failures, locked_until) VALUES(1, 0, 0);";
    char* err = NULL;
    int rc = sqlite3_exec(db->handle, ddl, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        pcretro_error(L"建表失败: %hs", err);
        sqlite3_free(err);
        return rc;
    }
    /* 默认设置 */
    const char* defaults[] = {
        "INSERT OR IGNORE INTO settings(key,value) VALUES('password_hash','')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('password_initialized','0')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('hotkey','ctrl+alt+p')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('silent_mode','0')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('boot_notify','1')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('ollama_url','http://127.0.0.1:11434')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('ollama_model','qwen2.5:7b')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('active_child_id','1')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('lockout_max_failures','5')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('lockout_seconds','300')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('service_installed','0')",
        "INSERT OR IGNORE INTO settings(key,value) VALUES('auto_start','1')",
        NULL,
    };
    for (int i = 0; defaults[i]; i++) {
        sqlite3_exec(db->handle, defaults[i], NULL, NULL, NULL);
    }
    /* 默认儿童档案 */
    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db->handle, "SELECT COUNT(*) FROM children", -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_int(st, 0) == 0) {
            sqlite3_exec(db->handle,
                "INSERT INTO children(name, created_at) VALUES('默认孩子', strftime('%s','now'))",
                NULL, NULL, NULL);
        }
        sqlite3_finalize(st);
    }
    return 0;
}

int pcretro_db_exec(pcretro_db_t* db, const char* sql) {
    char* err = NULL;
    int rc = sqlite3_exec(db->handle, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        pcretro_error(L"SQL 执行失败: %hs  SQL: %hs", err ? err : "?", sql);
        if (err) sqlite3_free(err);
    }
    return rc;
}

int64_t pcretro_db_last_insert_id(pcretro_db_t* db) {
    return sqlite3_last_insert_rowid(db->handle);
}

int pcretro_db_begin(pcretro_db_t* db) { return sqlite3_exec(db->handle, "BEGIN", NULL, NULL, NULL); }
int pcretro_db_commit(pcretro_db_t* db) { return sqlite3_exec(db->handle, "COMMIT", NULL, NULL, NULL); }
int pcretro_db_rollback(pcretro_db_t* db) { return sqlite3_exec(db->handle, "ROLLBACK", NULL, NULL, NULL); }

pcretro_db_t* pcretro_db_thread(void) {
    if (tls_db) return tls_db;
    pcretro_db_open(&tls_db);
    return tls_db;
}
