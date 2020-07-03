/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPBackground.h 
作者:   龙川
说明:   TCP后台管理头文件
***************************************************************************************************/
#ifndef __TCPBACKGROUND_H
#define __TCPBACKGROUND_H
#include "com.h"
#include "fifo.h"
#define TCP_PROTOCOL_VERSION  "V1.0.0"
#define BACKGROUND_SYNC_BYTE1 0xA5
#define BACKGROUND_SYNC_BYTE2 0xFF
typedef enum
{
	BYTE = 0,//单字节数据
	SHORT_LittleEndian = 1,//小端模式两字节
	SHORT_BigEndian = 2,//大端模式两字节
	INT_LittleEndian = 3,//小端模式四字节
	INT_BigEndian = 4,//大端模式四字节	
}DataFormat;
extern uint32_t TCPBackground_1msec;
extern uint32_t TCPBackground_1sec;
extern uint8_t  TCPBackground_1secFlag;
extern ComFifoStruct TCPBackGroundFIFO;//后台数据的存放FIFO

extern pthread_mutex_t TCPBackground_mutex;;

extern void TCPBackGroundInit(void);
extern void *TCPThread_Background(void * argument);
#endif

















