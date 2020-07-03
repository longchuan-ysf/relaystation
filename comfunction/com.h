#ifndef __COM_H
#define __COM_H
//包含linux内核的一些必要头文件
#include <linux/types.h>
#include <linux/kernel.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"
#include "linux/input.h"
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "log.h"


#endif // !__COM_H