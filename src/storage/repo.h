/* 仓库层：CRUD */
#ifndef PCRETRO_REPO_H
#define PCRETRO_REPO_H

#include "db.h"
#include "../common.h"

/* 事件插入 */
int64_t pcretro_repo_insert_event(
    const char* kind, const char* source, const char* title,
    const char* payload_json, int duration_ms, int child_id);

int pcretro_repo_insert_events_batch(const char* kind,
    const char* const* sources, const char* const* titles,
    const char* const* payloads, const int* durations, int n);

/* 事件查询 */
typedef struct {
    int64_t id;
    int64_t ts;
    char kind[32];
    char source[256];
    char title[512];
    char payload[1024];
    int64_t duration_ms;
    int child_id;
} pcretro_event_t;

int pcretro_repo_query_events(
    int64_t start_ts, int64_t end_ts,
    const char* const* kinds, int n_kinds,
    const char* keyword, int child_id, int limit,
    pcretro_event_t** out, int* out_count);
void pcretro_repo_free_events(pcretro_event_t* arr);

/* 统计 */
typedef struct {
    char source[256];
    int opens;
    int64_t total_ms;
    int64_t first_ts;
    int64_t last_ts;
} pcretro_stat_t;

int pcretro_repo_stats_by_app(int64_t start_ts, int64_t end_ts, int child_id,
    pcretro_stat_t** out, int* out_count);
void pcretro_repo_free_stats(pcretro_stat_t* arr);

/* 黑名单 */
int pcretro_repo_blacklist_add(const char* process_name, const char* display_name);
int pcretro_repo_blacklist_remove(const char* process_name);
typedef struct {
    int id;
    char process_name[256];
    char display_name[256];
    int64_t added_at;
    int enabled;
} pcretro_blacklist_t;
int pcretro_repo_blacklist_list(int enabled_only, pcretro_blacklist_t** out, int* out_count);
void pcretro_repo_blacklist_free(pcretro_blacklist_t* arr);

/* 限时 */
int pcretro_repo_time_limits_upsert(int child_id, int daily_limit_minutes,
    int weekday_mask, int start_minute, int end_minute, int enabled);
int pcretro_repo_time_limits_get(int child_id,
    int* daily_limit_minutes, int* weekday_mask, int* start_minute, int* end_minute, int* enabled);

/* 设置 KV */
const char* pcretro_repo_setting_get(const char* key);
int pcretro_repo_setting_set(const char* key, const char* value);
int pcretro_repo_setting_set_int(const char* key, int v);
int pcretro_repo_setting_get_int(const char* key, int defv);

/* 锁定 */
void pcretro_repo_lockout_get(int* failures, int64_t* locked_until);
void pcretro_repo_lockout_set(int failures, int64_t locked_until);

/* 儿童 */
typedef struct { int id; char name[64]; int64_t created_at; } pcretro_child_t;
int pcretro_repo_children_list(pcretro_child_t** out, int* out_count);
void pcretro_repo_children_free(pcretro_child_t* arr);
int pcretro_repo_children_add(const char* name);

/* 维护 */
int pcretro_repo_clear_events(void);
int pcretro_repo_vacuum(void);

#endif
