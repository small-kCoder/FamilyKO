#include "search.h"
#include "time_ranges.h"

int pcretro_search_events(pcretro_range_t* tr, const char* const* kinds, int n_kinds,
    const char* keyword, int child_id, int limit,
    pcretro_event_t** out, int* out_count)
{
    return pcretro_repo_query_events(tr->start_ts, tr->end_ts,
        kinds, n_kinds, keyword, child_id, limit, out, out_count);
}

int pcretro_search_stats(pcretro_range_t* tr, int child_id,
    pcretro_stat_t** out, int* out_count)
{
    return pcretro_repo_stats_by_app(tr->start_ts, tr->end_ts, child_id, out, out_count);
}
