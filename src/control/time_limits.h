/* 时长限制判定 */
#ifndef PCRETRO_TIME_LIMITS_H
#define PCRETRO_TIME_LIMITS_H

#include "../common.h"

typedef struct {
    bool allowed;
    wchar_t reason[64];
    int daily_used_sec;
    int daily_limit_sec;
    bool in_window;
} pcretro_limit_decision_t;

void pcretro_time_limits_evaluate(int child_id, pcretro_limit_decision_t* out);

#endif
