/* 时间段解析 */
#ifndef PCRETRO_TIMERANGES_H
#define PCRETRO_TIMERANGES_H

#include "../common.h"

typedef struct {
    int64_t start_ts;
    int64_t end_ts;
    wchar_t label[128];
} pcretro_range_t;

void pcretro_range_today(pcretro_range_t* r);
void pcretro_range_yesterday(pcretro_range_t* r);
void pcretro_range_last_night(pcretro_range_t* r);
void pcretro_range_this_week(pcretro_range_t* r);
void pcretro_range_last_7_days(pcretro_range_t* r);

#endif
