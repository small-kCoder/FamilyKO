#include "runner.h"
#include "app_usage.h"
#include "web_history.h"
#include "file_ops.h"

void pcretro_runner_start_all(void) {
    pcretro_app_usage_start();
    pcretro_web_history_start();
    pcretro_file_ops_start();
}

void pcretro_runner_stop_all(void) {
    pcretro_app_usage_stop();
    pcretro_web_history_stop();
    pcretro_file_ops_stop();
}
