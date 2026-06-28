/* 浏览器历史采集（Chrome/Edge/Firefox） */
#ifndef PCRETRO_WEB_HISTORY_H
#define PCRETRO_WEB_HISTORY_H

void pcretro_web_history_start(void);
void pcretro_web_history_stop(void);
int pcretro_web_history_poll_once(void);

#endif
