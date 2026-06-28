/* SQLite 数据库初始化与连接 */
#ifndef PCRETRO_DB_H
#define PCRETRO_DB_H

#include "../common.h"
#include <sqlite3.h>

typedef struct pcretro_db pcretro_db_t;

/* 打开/创建数据库。返回 0 成功，非 0 失败。 */
int pcretro_db_open(pcretro_db_t** out);
void pcretro_db_close(pcretro_db_t* db);

/* 初始化表结构（首次运行） */
int pcretro_db_init_schema(pcretro_db_t* db);

/* 同步执行一条无返回的 SQL（带 printf 风格参数） */
int pcretro_db_exec(pcretro_db_t* db, const char* sql);

/* 取最后插入的 rowid */
int64_t pcretro_db_last_insert_id(pcretro_db_t* db);

/* 事务 */
int pcretro_db_begin(pcretro_db_t* db);
int pcretro_db_commit(pcretro_db_t* db);
int pcretro_db_rollback(pcretro_db_t* db);

/* 线程安全：每个线程调用此函数获取独立连接 */
pcretro_db_t* pcretro_db_thread(void);

#endif
