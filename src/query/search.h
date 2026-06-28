/* 查询封装 */
#ifndef PCRETRO_SEARCH_H
#define PCRETRO_SEARCH_H

#include "../common.h"
#include "../storage/repo.h"

int pcretro_search_events(pcretro_range_t* tr, const char* const* kinds, int n_kinds,
    const char* keyword, int child_id, int limit,
    pcretro_event_t** out, int* out_count);

int pcretro_search_stats(pcretro_range_t* tr, int child_id,
    pcretro_stat_t** out, int* out_count);

#endif
