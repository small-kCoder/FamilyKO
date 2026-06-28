/* 测试：时间范围解析 */
#include "query/time_ranges.h"
#include "common.h"

#include <stdio.h>
#include <assert.h>

static void check(bool cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
    } else {
        printf("PASS: %s\n", msg);
    }
}

int main(void) {
    pcretro_range_t r;

    pcretro_range_today(&r);
    check(r.start_ts > 0 && r.end_ts > r.start_ts, "today: end > start");
    check(r.end_ts - r.start_ts >= 23 * 3600, "today: 跨度至少 23h");

    pcretro_range_yesterday(&r);
    check(r.end_ts - r.start_ts == 24 * 3600, "yesterday: 跨度为 24h");
    check(r.start_ts > 0, "yesterday: start > 0");

    pcretro_range_last_night(&r);
    check(r.end_ts - r.start_ts == 6 * 3600, "last_night: 跨度为 6h");

    pcretro_range_this_week(&r);
    check(r.end_ts - r.start_ts == 7 * 24 * 3600, "this_week: 跨度为 7 天");

    pcretro_range_last_7_days(&r);
    check(r.end_ts - r.start_ts == 7 * 24 * 3600, "last_7_days: 跨度为 7 天");

    printf("\n所有时间范围测试通过\n");
    return 0;
}
