#include "time_ranges.h"
#include <time.h>

static void make_range(pcretro_range_t* r, struct tm* start, struct tm* end, const wchar_t* label) {
    r->start_ts = (int64_t)mktime(start);
    r->end_ts = (int64_t)mktime(end);
    wcsncpy(r->label, label, 127);
    r->label[127] = 0;
}

void pcretro_range_today(pcretro_range_t* r) {
    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    struct tm s = t; s.tm_hour = 0; s.tm_min = 0; s.tm_sec = 0;
    struct tm e = s; e.tm_mday += 1;
    make_range(r, &s, &e, L"今天");
}

void pcretro_range_yesterday(pcretro_range_t* r) {
    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    struct tm s = t; s.tm_hour = 0; s.tm_min = 0; s.tm_sec = 0; s.tm_mday -= 1;
    struct tm e = s; e.tm_mday += 1;
    make_range(r, &s, &e, L"昨天");
}

void pcretro_range_last_night(pcretro_range_t* r) {
    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    struct tm s = t; s.tm_hour = 0; s.tm_min = 0; s.tm_sec = 0; s.tm_mday -= 1; s.tm_hour = 18;
    struct tm e = s; e.tm_mday += 1; e.tm_hour = 0;
    make_range(r, &s, &e, L"昨晚 18:00 - 今日 00:00");
}

void pcretro_range_this_week(pcretro_range_t* r) {
    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    struct tm s = t; s.tm_hour = 0; s.tm_min = 0; s.tm_sec = 0; s.tm_mday -= t.tm_wday == 0 ? 6 : (t.tm_wday - 1);
    struct tm e = s; e.tm_mday += 7;
    make_range(r, &s, &e, L"本周");
}

void pcretro_range_last_7_days(pcretro_range_t* r) {
    time_t now = time(NULL);
    struct tm t; localtime_s(&t, &now);
    struct tm s = t; s.tm_hour = 0; s.tm_min = 0; s.tm_sec = 0; s.tm_mday -= 6;
    struct tm e = s; e.tm_mday += 7;
    make_range(r, &s, &e, L"最近 7 天");
}
