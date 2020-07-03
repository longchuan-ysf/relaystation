/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   NetHandle.c 
作者:   龙川
说明:   网络处理相关

***************************************************************************************************/
#include "NetHandle.h"
#include "fifo.h"
#include "fun.h"
#include "EquipmentManage.h"
#include "TCPModbus.h"
#include "TCPModbus_M.h"
#include "log.h"
 /**
 ****************************************************************************************
 @brief:	TCP_Client_Send_Data TCP客户端发送数据函数
 @Input：	buff 数据
			length 数据长度
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-7-9
 ****************************************************************************************
  **/
int TCPModbus_Client_Send_Data(uint8_t *buff,uint32_t length)
{
	int ret;
	if(EquipmentManage.enet_connect_state == true)
	{
		if(EquipmentManage.tcp_modbus.tcp_connect_state != false)
		{
			ret = write(EquipmentManage.tcp_modbus.sockfd, buff, length);
			
			if(ret <= 0)
			{
				//客户端掉线
				dzlog_error("write err!\r\n");
				EquipmentManage.tcp_modbus.tcp_connect_state = false;
				return ret;
			}
		}
	}
	return 0;			
}


int  Modbus_Client_Recv(void)
{
	char recvbuf[1024]={0};
	
	if(EquipmentManage.tcp_modbus.tcp_connect_state != false)
	{
		dzlog_debug("%s tcp modbus read\r\n",__func__);
		int ret = read(EquipmentManage.tcp_modbus.sockfd, recvbuf, 1024);//阻塞
		if(ret <= 0)
		{
			//客户端掉线
			dzlog_warn("%s tcp modbus read err!\r\n",__func__);
			EquipmentManage.tcp_modbus.tcp_connect_state = false;
			return -1;
		}
		else
		{
			fifo_in(&TCPModbusFIFO.rxFifo,(uint8_t *)recvbuf,ret);
		}
		
		
	}
	return 0;
}
 /**
 ****************************************************************************************
 @brief:	TCPModbus_Client_Init 后台TCP客户端初始化
 @Input：	serverport 服务器端口号
			serverip 服务器IP
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-7-9
 ****************************************************************************************
  **/
int TCPModbus_Client_Init(uint8_t *serverport,uint8_t *serverip)
{
	//debugf("%s net state %s!\r\n",__func__,EquipmentManage.enet_connect_state == true?"true":"false");
		static int reconnect = 1000;
		//不用频繁的重连每秒重连，连不上
		if(reconnect)
		{
			reconnect--;
			return -1;
		}
		reconnect=1000;
		if(EquipmentManage.tcp_modbus.tcp_connect_state != true)
		{
			if(EquipmentManage.tcp_modbus.sockfd)
			{
				dzlog_debug("%s close socket %d!\r\n",__func__,EquipmentManage.tcp_modbus.sockfd);
				close(EquipmentManage.tcp_modbus.sockfd);
			}
			EquipmentManage.tcp_modbus.sockfd = 0;
			//1.创建套接字
			EquipmentManage.tcp_modbus.sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if(EquipmentManage.tcp_modbus.sockfd < 0)
			{
				dzlog_error("socket fail\r\n");
				return -1;
			}
			dzlog_debug("%s socket ok id = %d\r\n",__func__,EquipmentManage.tcp_modbus.sockfd);
			//2.连接服务器
			struct sockaddr_in servaddr;
			bzero(&servaddr, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(atoi((char *)serverport));
			servaddr.sin_addr.s_addr = inet_addr((char *)serverip);

			int ret = connect(EquipmentManage.tcp_modbus.sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
			if(ret < 0)
			{
				dzlog_error("%s connect fail:\r\n",__func__);
				close(EquipmentManage.tcp_modbus.sockfd);
				EquipmentManage.tcp_modbus.sockfd = 0;
				EquipmentManage.tcp_modbus.tcp_connect_state = false;
				return -1;
			}
			else
			{
				dzlog_error("tcp modbus connect ok!\r\n");
				
				EquipmentManage.tcp_modbus.tcp_connect_state = true;
			}
			
		}
	

	return 0;
}

 /**
 ****************************************************************************************
 @brief:	Backgroun_Check_TCP_Connect 检查连接
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-7-9
 ****************************************************************************************
  **/

void Modbus_Check_TCP_Connect(void)
{
	uint8_t ModbusData[16] = {0};

	if(EquipmentManage.tcp_modbus.tcp_connect_state == false)
	{
		TCPModbus_Client_Init(EquipmentManage.tcp_modbus.serverport,EquipmentManage.tcp_modbus.serverip); //重连
		if(EquipmentManage.tcp_modbus.tcp_connect_state == true)
		{
			//重连之后清掉之前的计数
			TCPModbus_1msec = 0;
			TCPModbus_1sec = 0;
			TCPModbus_1secFlag = 0;
			sleep(2);
			dzlog_debug("clear PLC reg!\r\n");
			MODH_WriteParam_10H(TCPModbusRegArray[CookBookStart].reg_addr,2,ModbusData);
			MODH_WriteParam_10H(TCPModbusRegArray[CookFinishStart].reg_addr,4,ModbusData);
		}
	}
	return ;	
}

