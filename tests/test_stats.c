/* 测试：统计聚合 */
#include "storage/db.h"
#include "storage/repo.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

static void check(bool cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
    } else {
        printf("PASS: %s\n", msg);
    }
}

int main(void) {
    /* 使用临时数据库 */
    putenv("PCRETRO_DATA_DIR=C:\\Temp\\PCRetroTestStats");
    CreateDirectoryW(L"C:\\Temp\\PCRetroTestStats", NULL);

    pcretro_db_t* db = NULL;
    int rc = pcretro_db_open(&db);
    check(rc == 0 && db, "打开数据库");
    pcretro_db_init_schema(db);

    /* 清空 */
    pcretro_repo_clear_events();

    /* 插入 3 条 app 事件 */
    int64_t now = pcretro_now();
    for (int i = 0; i < 3; i++) {
        pcretro_repo_insert_event("app", "notepad.exe", "test", "{}", 60000, 0);
    }
    pcretro_repo_insert_event("app", "chrome.exe", "google", "{}", 30000, 0);

    /* 统计 */
    pcretro_stat_t* arr = NULL; int n = 0;
    rc = pcretro_repo_stats_by_app(now - 60, now + 60, 0, &arr, &n);
    check(rc == 0, "stats_by_app 返回 0");
    check(n == 2, "返回 2 个应用");

    /* notepad 应有 3 次，180000ms */
    int np_idx = -1, ch_idx = -1;
    for (int i = 0; i < n; i++) {
        if (strcmp(arr[i].source, "notepad.exe") == 0) np_idx = i;
        if (strcmp(arr[i].source, "chrome.exe")  == 0) ch_idx = i;
    }
    check(np_idx >= 0, "找到 notepad.exe");
    check(ch_idx >= 0, "找到 chrome.exe");
    if (np_idx >= 0) check(arr[np_idx].opens == 3, "notepad 3 次");
    if (ch_idx >= 0) check(arr[ch_idx].opens == 1, "chrome 1 次");

    pcretro_repo_free_stats(arr);
    pcretro_db_close(db);

    printf("\n所有统计测试通过\n");
    return 0;
}
