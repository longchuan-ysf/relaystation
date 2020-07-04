#ifndef __LOG_H_
#define __LOG_H_

#include <stdio.h>
#include "zlog.h"


extern int zloginit(void);
extern void zlogdeinit(void);
extern void zlog_scan_oldest(void);
#endif // !__LOG_H_
