#include "com.h"
#include "EquipmentManage.h"
#include "TCPBackground.h"
#include "TCPModbus.h"
#include "Record.h"
#include "log.h"
int net_detect(char* net_name)
{

    int skfd = 0;

    struct ifreq ifr;
	alarm(3);
	//printf("check net connect state!\r\n");
    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(skfd < 0)
    {
        dzlog_error("%s:%d Open socket error!\n", __func__, __LINE__);
        return -1;
    }

    strcpy(ifr.ifr_name, net_name);

    if(ioctl(skfd, SIOCGIFFLAGS, &ifr) <0 )

    {

        dzlog_error("IOCTL error!\n");

        dzlog_error("Maybe ethernet inferface %s is not valid!", ifr.ifr_name);

        close(skfd);

        return -1;

    }

    if(ifr.ifr_flags & IFF_RUNNING)
    {
       // printf("%s is running :)\n", ifr.ifr_name);
    }
    else
    {
        //printf("%s is not running :(\n", ifr.ifr_name);
		close(skfd);
		return -1;
    }
	close(skfd);
    return 0;
}
//定时检测网线是否被拔掉
static void net_detect_alarm(int arg)
{
	int ret;
	ret = net_detect("eth0");
	if(ret < 0)
	{
		EquipmentManage.enet_connect_state = false;
		EquipmentManage.tcp_background.tcp_connect_state = false;
		EquipmentManage.tcp_modbus.tcp_connect_state = false;
	}
	else
	{
		EquipmentManage.enet_connect_state = true;
	}
	
}
static void Refrigertor_state_indicate()
{
	//一旦检测过就不让蜂鸣器叫防止检测到一个通道小于3份，重复叫蜂鸣器
	static uint8_t check[MAX_REFRIGERTOR_ROW][MAX_REFRIGERTOR_COLUMN] = {0};
	//蜂鸣器鸣叫时长
	static uint16_t beep_time = 0;
	//对上面的check标志清零的周期，让机器重新检测通道
	static uint32_t clear_time = 3600*23;//23小时晴空一次标志
	static uint8_t beep_flag = 0;//0开机状态，1检测到需要鸣叫然后设置鸣叫，2鸣叫计时计时完成清掉标志
	uint8_t row,column;

	if(clear_time)
	{
		clear_time--;
	}
	else
	{
		clear_time = 3600*20;
		memset(check,0,sizeof(check));
	}
	//冷柜还没有同步，退出
	if(EquipmentManage.RefrigertorRefresh == false)
		return;
	switch (beep_flag)
	{
		case 0:
		{
			//查询
			for(row=0; row<MAX_REFRIGERTOR_ROW; row++)
			{
				for(column=0; column<MAX_REFRIGERTOR_COLUMN; column++)
				{
					//如果该通道没有检测过并且标识缺货
					if(!check[row][column])
					{
						if((ReFoodMaterial[row][column].num<=3)&&(atoi((char *)ReFoodMaterial[row][column].SN)))
						{
							dzlog_debug("row %d,column %d SN %s num %d\r\n",row,column,\
							ReFoodMaterial[row][column].SN,ReFoodMaterial[row][column].num);
							check[row][column] = 1;
							beep_flag = 1;	
						}
					}
				}
			}
		}
		break;
		case 1:
		{
			system("echo heartbeat > /sys/class/leds/beep/trigger");	
			beep_time = 20;//蜂鸣器鸣叫20s
			beep_flag = 2;
		}
		break;
		case 2:
		{
			if(beep_time)
			{
				beep_time--;
			}
			else
			{
				beep_time = 0;
				beep_flag = 0;
				system("echo none > /sys/class/leds/beep/trigger");
			}
		}
		break;
	
	default:
		break;
	}
	
	

	
	
}
static void connet_state_indicate()
{

	static uint8_t state = 0,statebackup = -1;

	state = (EquipmentManage.tcp_modbus.tcp_connect_state&0x01)|\
			((EquipmentManage.tcp_background.tcp_connect_state&0x01)<<1);
	if(statebackup != state)
	{
		statebackup = state;
		dzlog_debug("conect state = %d\r\n",state);
		switch(state)
		{
			case 0:
			{

			}
			//break;
			case 1:
			{

			}
			//break;
			case 2:
			{
				system("echo timer > /sys/class/leds/sys-led/trigger");
			}
			break;
			case 3:
			{
				system("echo heartbeat > /sys/class/leds/sys-led/trigger");
			}
			break;
			default:
			{

			}
			break;

		}
	}
}

int main(int argc, char **argv)
{
	//创建定时器，用来检测网线是否被拔掉
	signal(SIGALRM,net_detect_alarm);
	alarm(3);
	zloginit();
	SysParaPowerOnInit();
	
	//RefrigertorPowerOnInit();
	EquipmentManagerInit();
	
	//启动线程用来专门读取数据
	pthread_t TCPBackgroundID,EquipmemtManageID,TCPModbusID;
	pthread_create(&EquipmemtManageID, NULL, CookerManage,NULL);
	pthread_detach(EquipmemtManageID);//主线成和子线程分离，线程结束各自自动回收资源

	pthread_create(&TCPBackgroundID, NULL, TCPThread_Background,NULL);
	pthread_detach(TCPBackgroundID);
	
	pthread_create(&TCPModbusID, NULL, TCPThread_Modbus,NULL);
	pthread_detach(TCPModbusID);
	//一些杂项就放到下面的循环中
	while(1)
	{	
		//每秒检测下系统连接情况然后通过蜂鸣器提示
		connet_state_indicate();
		Refrigertor_state_indicate();
		zlog_scan_oldest();
		sleep(1);
	}
	zlogdeinit();
	return 0;
}
