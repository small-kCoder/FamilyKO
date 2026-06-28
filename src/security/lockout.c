#include "lockout.h"
#include "../storage/repo.h"

bool pcretro_lockout_is_locked(int* remain_sec) {
    int failures;
    int64_t until;
    pcretro_repo_lockout_get(&failures, &until);
    int64_t now = pcretro_now();
    if (until && now < until) {
        if (remain_sec) *remain_sec = (int)(until - now);
        return true;
    }
    if (remain_sec) *remain_sec = 0;
    return false;
}

void pcretro_lockout_record_failure(void) {
    int failures;
    int64_t until;
    pcretro_repo_lockout_get(&failures, &until);
    int max = pcretro_repo_setting_get_int("lockout_max_failures", 5);
    int sec = pcretro_repo_setting_get_int("lockout_seconds", 300);
    failures++;
    if (failures >= max) {
        until = pcretro_now() + sec;
        failures = 0;
    } else {
        until = 0;
    }
    pcretro_repo_lockout_set(failures, until);
}

void pcretro_lockout_reset(void) {
    pcretro_repo_lockout_set(0, 0);
}
