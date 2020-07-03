#include "log.h"
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