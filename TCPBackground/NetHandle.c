/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   NetHandle.c 
作者:   龙川
说明:   网络处理相关

***************************************************************************************************/
#include "NetHandle.h"
#include "fifo.h"
#include "EquipmentManage.h"
#include "TCPBackground.h"
#include "TCPMessageHandle.h"
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
int TCP_Client_Send_Data(uint8_t *buff,uint32_t length)
{
	int ret;
	if(EquipmentManage.enet_connect_state == true)
	{
		if(EquipmentManage.tcp_background.tcp_connect_state != false)
		{
			ret = write(EquipmentManage.tcp_background.sockfd, buff, length);
			
			if(ret <= 0)
			{
				//客户端掉线
				dzlog_error("write err!\r\n");
				EquipmentManage.tcp_background.tcp_connect_state = false;
				return ret;
			}
		}
	}
	return 0;			
}


int  TCP_Client_Recv(void)
{
	char recvbuf[1024]={0};
	
	if(EquipmentManage.tcp_background.tcp_connect_state != false)
	{
		dzlog_debug("%s,read\r\n",__func__);
		int ret = read(EquipmentManage.tcp_background.sockfd, recvbuf, 1024);//阻塞
		if(ret <= 0)
		{
			//客户端掉线
			dzlog_error("tcp background read err!\r\n");
			EquipmentManage.tcp_background.tcp_connect_state = false;
			return -1;
		}
		else
		{
			// debugf("tcp background read ok\r\n");
			// debugf_Hexbuffer(recvbuf,ret);	
			fifo_in(&TCPBackGroundFIFO.rxFifo,(uint8_t *)recvbuf,ret);
		}
		
		
	}
	return 0;
}
 /**
 ****************************************************************************************
 @brief:	TCP_Client_Background_Init 后台TCP客户端初始化
 @Input：	serverport 服务器端口号
			ServerIP 服务器IP
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-7-9
 ****************************************************************************************
  **/
int TCP_Client_Background_Init(uint8_t *serverport,uint8_t *serverip)
{
	
	if(EquipmentManage.tcp_background.tcp_connect_state != true)
	{
		static int reconnect = 1000;
		
		//不用频繁的重连每秒重连，连不上
		if(reconnect)
		{
			reconnect--;
			return -1;
		}
		reconnect = 1000;
		TCP_MessageFlag.SignIn=0;
		if(EquipmentManage.tcp_background.sockfd)
		{
			dzlog_debug("%s %d close socket %d!\r\n",__func__,__LINE__,EquipmentManage.tcp_background.sockfd);
			close(EquipmentManage.tcp_background.sockfd);
		}
		EquipmentManage.tcp_background.sockfd = 0;
		//1.创建套接字
		EquipmentManage.tcp_background.sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(EquipmentManage.tcp_background.sockfd < 0)
		{
			dzlog_error("%s socket fail\r\n",__func__);
			return -1;
		}
		dzlog_debug("%s socket ok id = %d\r\n",__func__,EquipmentManage.tcp_background.sockfd);
		//2.连接服务器
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(atoi((char *)serverport));
		servaddr.sin_addr.s_addr = inet_addr((char *)serverip);

		int ret = connect(EquipmentManage.tcp_background.sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		if(ret < 0)
		{
			dzlog_error("%s connect fail:\r\n",__func__);
			close(EquipmentManage.tcp_background.sockfd);
			EquipmentManage.tcp_background.sockfd = 0;
			EquipmentManage.tcp_background.tcp_connect_state = false;
			return -1;
		}
		else
		{
			
			CMD206_Handle();
			EquipmentManage.tcp_background.tcp_connect_state = true;
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

void Background_Check_TCP_Connect(void)
{
	
	if(EquipmentManage.tcp_background.tcp_connect_state == false)
	{
		TCP_Client_Background_Init(EquipmentManage.tcp_background.serverport,EquipmentManage.tcp_background.serverip); //重连
	}
	return ;	
}

