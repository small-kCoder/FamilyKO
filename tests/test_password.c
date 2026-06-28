/* 测试：密码哈希与校验 */
#include "security/password.h"
#include "common.h"
#include "storage/db.h"

#include <stdio.h>
#include <stdlib.h>

static void check(bool cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
    } else {
        printf("PASS: %s\n", msg);
    }
}

int main(void) {
    putenv("PCRETRO_DATA_DIR=C:\\Temp\\PCRetroTestPwd");
    CreateDirectoryW(L"C:\\Temp\\PCRetroTestPwd", NULL);

    pcretro_db_t* db = NULL;
    pcretro_db_open(&db);
    pcretro_db_init_schema(db);

    /* 强度检查 */
    check(!pcretro_password_is_strong(L"abc"),   "弱：太短");
    check(!pcretro_password_is_strong(L"abcdefgh"), "弱：缺数字");
    check(!pcretro_password_is_strong(L"12345678"), "弱：缺字母");
    check(pcretro_password_is_strong(L"abc12345"),  "强");

    /* 设置并验证 */
    check(pcretro_password_set(L"Test1234") == 0, "密码设置成功");
    check(pcretro_password_is_initialized(),        "密码已初始化");
    check(pcretro_password_verify(L"Test1234"),     "正确密码通过");
    check(!pcretro_password_verify(L"Wrong1"),     "错误密码拒绝");
    check(!pcretro_password_verify(L"test1234"),    "区分大小写");

    pcretro_db_close(db);
    printf("\n所有密码测试通过\n");
    return 0;
}
