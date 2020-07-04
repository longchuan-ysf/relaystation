#include "log.h"
#include "com.h"
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#define MAX_LOGFILE_DATE 5
//zlog的使用方法详见https://www.cnblogs.com/lsgxeva/p/9486414.html
//目前程序中只是用一个分类 然后使用4个等级
//error打开文件出错等
//warn机器下线信息 某些设备下线信息
//info重点关注的的debug信息
//debug调试信息
int zloginit(void)
{
	int rc;
	rc = dzlog_init("./log.conf", "my_cat"); //指定配置文件路径及类型名 初始化zlog
	if (rc)
	{
		printf("init failed\n");
		return -1;
	}
	dzlog_error("hello, zlog error\r\n");//打印错误信息
	
	dzlog_warn("hello, zlog warning\r\n"); //打印报警信息

    dzlog_info("hello, zlog info\r\n"); //打印普通信息
	
	dzlog_debug("hello, zlog debug\r\n"); //打印调试信息
	
	
	return 0;
}
void zlogreconfig(char *config)
{
    zlog_reload(config);
}
void zlogdeinit(void)
{
    zlog_fini();  //释放zlog
}

 //扫描日志文件,超过5天的就删除
void zlog_scan_oldest(void)
{
	static int exe=0;
	static time_t time_backup = 0,time_clr;
	struct tm *date;
	time_t time_s;
    char cmd[128] = {0};
	int i;
	char * path[]={"./log/debug","./log/info","./log/warn","./log/error"};
	time_s = time(NULL);
	//保证这个函数至少每秒运行一次——如果调用的是2s一次那就是2s一次
	if(time_s != time_backup)
	{
		time_backup = time_s;
	}
	else
	{
		return;
	}
	
	date = localtime(&time_s);
	//这个程序每天执行一次，执行完之后不断检测时间，过来当天后再次执行
	if(exe)
	{
		//因为下面不允许用date->tm_sec == 0来清，这样如果错过那个0s这一天就不会清记录了
		if((time_s-time_clr)>60)//清零之后要超过1分钟之后才允许再次判断
		{
			if((date->tm_hour==0)&&(date->tm_min==0))
			{
				exe = 0;
				time_clr = time_s;//记录当前清零的时间戳
				dzlog_info("clear exe at %d\r\n",time_clr);
			}
		}
		return;
	}
	for(i=0; i<sizeof(path)/sizeof(char *);i++)
	{
		//删除上一次修改时间超过MAX_LOGFILE_DATE天的文件,注意find命令的+n是算n-1天之前的，详细百度find命令
		sprintf(cmd, "find %s/* -mtime +%d -exec rm -rf {} \\;", path[i],MAX_LOGFILE_DATE-1);
		dzlog_info("%s\r\n",cmd);   
		system(cmd);
	}
	exe = 1;
}
