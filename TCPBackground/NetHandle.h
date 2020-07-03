/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   NetHandle.h 
作者:   龙川
说明:   网络处理相关

***************************************************************************************************/

#ifndef __TCPNETHANDLE_H
#define __TCPNETHANDLE_H
#include "com.h"

extern int TCP_Client_Send_Data(uint8_t *buff,uint32_t length);
extern int TCP_Client_Recv(void);
extern int TCP_Client_Background_Init(uint8_t *serverport,uint8_t *serverip);
extern void Background_Check_TCP_Connect(void);
#endif


