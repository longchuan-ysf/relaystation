/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   NetHandle.h 
作者:   龙川
说明:   网络处理相关

***************************************************************************************************/

#ifndef __TCPMODBUSNETHANDLE_H
#define __TCPMODBUSNETHANDLE_H


extern int TCPModbus_Client_Send_Data(uint8_t *buff,uint32_t length);
extern int  Modbus_Client_Recv(void);
extern int TCPModbus_Client_Init(uint8_t *serverport,uint8_t *serverip);
extern void Modbus_Check_TCP_Connect(void);

#endif


