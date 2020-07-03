/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPMessageHandle.h
作者:   龙川
说明:   TCP后台报文处理相关

***************************************************************************************************/

#ifndef __TCPMESSAGEHANDLE_H
#define __TCPMESSAGEHANDLE_H
#include "EquipmentManage.h"
typedef struct
{
	uint8_t Head[2];//两个字节的头
	uint8_t Length_L;//长度低字节
	uint8_t Length_H;//长度高字节
	uint8_t MessageSN;//序列号
	uint8_t Information ;//信息域
	uint8_t CMD[2];//命令代码
	uint8_t *data;//数据域——此处的数据域是指向数据域的指针
	//uint8_t CheckSum;//由于数据域数据长度不定所以不能定义校验和的位置
	
}TCP_Message;

typedef struct
{
	uint8_t RSV[2];//预留
	uint8_t Ecrypt;//加密
	uint8_t EquipmentSn[16];//设备编号
	uint8_t PCProtocolVer[16];//PC端网络协议版本
	uint8_t PCSoftwareVer[16] ;//PC端软件版本
	uint8_t MasterProtocolVer[16] ;//主控网络协议版本
	uint8_t MasterSoftwareVer[16] ;//主控软件版本
	uint8_t MasterCANVer[16] ;//主控CAN协议版本
	uint8_t TotolOrder[4];//总订单数
	uint8_t RunMode;//运行模式
	uint8_t Random[4];//随机数
	uint8_t HeartbeatFre[4];//心跳包周期
	uint8_t HeartbeatTimeout[4];//心跳包超时次数
	//uint8_t CheckSum;//由于数据域数据长度不定所以不能定义校验和的位置
	
}TCP_CMD206Struct;

//32位
typedef struct
{
	uint32_t SignIn:1;//签到成功
	uint32_t HeartBeat;//心跳
	
}TCP_FlagStruct;


typedef struct
{
	uint8_t row;//行
	uint8_t column;//列
	uint8_t num;//个数
	uint8_t FoodMaterial[16];//编号

}RefrigertorFoodSnStruct;

extern TCP_FlagStruct TCP_MessageFlag;
extern void CMD116_Handle(OrderInfoStruct *pOrderinfo,uint8_t Cooker);
extern void CMD112_Handle(OrderInfoStruct *pOrderinfo);
extern void CMD108_Handle(uint8_t *orderSn,uint8_t State);
extern void TCPMessageHandle(void);
extern void CMD206_Handle(void);
extern void CMD20C_Handle(uint8_t row ,uint8_t column,uint16_t FoodMaterial);
void CMD124_Handle(uint8_t *OrderSn,uint8_t pan_num);

#endif








