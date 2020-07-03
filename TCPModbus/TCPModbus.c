
/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPModbus.c 
作者:   龙川
说明:   TCP后台管理源文件

***************************************************************************************************/
#include "TCPModbus.h"
#include "TCPModbus_M.h"
#include "TCPBackground.h"
#include "fifo.h"
#include "EquipmentManage.h"
#include "TCPModbusNetHandle.h"



uint32_t TCPModbus_1msec;
uint32_t TCPModbus_1sec;
uint8_t  TCPModbus_1secFlag;

ComFifoStruct TCPModbusFIFO;//TCP modbus数据的存放FIFO
//Modbus数据发送区互斥锁
pthread_mutex_t TCPModbus_mutex;

#define TCPModbus_TxBufferLen  128
#define TCPModbus_RxBufferLen  128

uint8_t TCPModbus_TXBuffer[TCPModbus_TxBufferLen];
uint8_t TCPModbus_RXBuffer[TCPModbus_RxBufferLen];


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
void TCPModbusInit(void)
{
	//使用二值信号量，防止多线程并发访问导致数据出错
	memset(&g_tModH,0,sizeof(g_tModH));
	memset(&TCPModbusFIFO,0,sizeof(TCPModbusFIFO));
	TCPModbusFIFO.rxFifo.len = TCPModbus_RxBufferLen;
	TCPModbusFIFO.rxFifo.data = TCPModbus_RXBuffer;
	TCPModbusFIFO.txFifo.len = TCPModbus_TxBufferLen;
	TCPModbusFIFO.txFifo.data = TCPModbus_TXBuffer;
	TCPModbus_1msec = 0;
	TCPModbus_1sec = 0;
	TCPModbus_1secFlag = 0;

	ModbusTCPTxDataHead.pHead = NULL;
	ModbusTCPTxDataHead.pTail = NULL;
	ModbusTCPTxDataHead.ListLen = 0;
	pthread_mutex_init(&TCPModbus_mutex,NULL);
	memset(&g_tModH,0,sizeof(g_tModH));
	g_tModH.MODTCPHead = rand()&0xffff;
	
}

static void *recv_handler(void *arg)
{
	dzlog_debug("modbus");
	while(1)
	{
		Modbus_Client_Recv();
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
void *TCPThread_Modbus(void * argument)
{
	dzlog_debug("TCPThread_Modbus");
	TCPModbusInit();
	//TCPModbus_Client_Init(EquipmentManage.tcp_modbus.serverport,EquipmentManage.tcp_modbus.serverip);//建立连接 
	pthread_t id =0;
	pthread_create(&id, NULL, recv_handler,NULL);
	pthread_detach(id);
	while(1)
	{
		Modbus_Check_TCP_Connect();	
		TCPModbus_1msec++;
		if(TCPModbus_1msec%1000 == 0)
		{
			TCPModbus_1sec++;
			TCPModbus_1secFlag = 1;
		}
		ModbusTCP_Cyclic_Communication();
		usleep(1000);		
		TCPModbus_1secFlag = 0;
	}
  /* USER CODE END 5 */ 
}













