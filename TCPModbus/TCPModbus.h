/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPBackground.h 
作者:   龙川
说明:   TCPModbus管理头文件
***************************************************************************************************/
#ifndef __TCPMODBUS_H
#define __TCPMODBUS_H
#include "com.h"
#include "fifo.h"


extern uint32_t TCPModbus_1msec;
extern uint32_t TCPModbus_1sec;
extern uint8_t  TCPModbus_1secFlag;

extern ComFifoStruct TCPModbusFIFO;//TCP modbus数据的存放FIFO

extern pthread_mutex_t TCPModbus_mutex;

extern void TCPBackGroundInit(void);
extern void *TCPThread_Modbus(void * argument);
#endif

















