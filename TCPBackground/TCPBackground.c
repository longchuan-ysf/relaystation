
/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPBackground.c 
作者:   龙川
说明:   TCP后台管理源文件

***************************************************************************************************/
#include "TCPBackground.h"
#include "fifo.h"
#include "EquipmentManage.h"
#include "NetHandle.h"
#include "TCPMessageHandle.h"
ComFifoStruct TCPBackGroundFIFO;//后台数据的存放FIFO


uint32_t TCPBackground_1msec;
uint32_t TCPBackground_1sec;
uint8_t  TCPBackground_1secFlag;

//后台发送数据区互斥锁，防止数据发送出去前被其他线程修改
pthread_mutex_t TCPBackground_mutex;

#define TCPBackGround_TxBufferLen  1024
#define TCPBackGround_RxBufferLen  4096

uint8_t TCPBackGround_TXBuffer[TCPBackGround_TxBufferLen];
uint8_t TCPBackGround_RXBuffer[TCPBackGround_RxBufferLen];
uint16_t TCPBackGroundTXDataLen;

/**
****************************************************************************************
@brief:    TCPBackGroundInit 后台FIFO初始化
@Input：   NULL
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-4-18
****************************************************************************************
 **/
void TCPBackGroundInit(void)
{
	memset(&TCPBackGroundFIFO,0,sizeof(TCPBackGroundFIFO));
	TCPBackGroundFIFO.rxFifo.len = TCPBackGround_RxBufferLen;
	TCPBackGroundFIFO.rxFifo.data = TCPBackGround_RXBuffer;
	TCPBackGroundFIFO.txFifo.len = TCPBackGround_TxBufferLen;
	TCPBackGroundFIFO.txFifo.data = TCPBackGround_TXBuffer;
	TCP_MessageFlag.SignIn = 0;
	pthread_mutex_init(&TCPBackground_mutex,NULL);
}

static void *recv_handler(void *arg)
{
	dzlog_debug("%s\r\n",__func__);
	while(1)
	{	
		TCP_Client_Recv();
	}
	return 0;
}
 /**
 ****************************************************************************************
 @brief:	TCPThread_Background TCP后台处理
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-7-6
 ****************************************************************************************
  **/
void *TCPThread_Background(void * argument)
{
	dzlog_debug("%s\r\n",__func__);
	TCPBackGroundInit();
	//TCP_Client_Background_Init(EquipmentManage.tcp_background.serverport,EquipmentManage.tcp_background.serverip);//建立连接 
	pthread_t id =0;
	pthread_create(&id, NULL, recv_handler,NULL);
	pthread_detach(id);
	while(1)
	{
		TCPBackground_1msec++;
		if(TCPBackground_1msec%1000 == 0)
		{
			TCPBackground_1sec++;
			TCPBackground_1secFlag = 1;
		}
		Background_Check_TCP_Connect();	
		TCPMessageHandle();
		usleep(1000);
		TCPBackground_1secFlag = 0;
		
	}
	pthread_mutex_destroy(&TCPBackground_mutex);
	return 0;
  /* USER CODE END 5 */ 
}













