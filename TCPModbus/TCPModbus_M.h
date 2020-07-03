/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPModbus_M.h 
作者:   龙川
说明:   TCPModbus管理头文件
***************************************************************************************************/
#ifndef __TCPMODBUS_M_H
#define __TCPMODBUS_M_H
#include "com.h"
#include "fifo.h"

//PLC一些寄存器索引起始位置
#define InductionCookerWarnStart 0 
#define CookerMotorWarnStart 4
#define RefrigertorMotorWarnStart 8
#define ScramStart 9
#define CookingErr 10
#define CookerState 11
#define RefrigertorState 15
#define CookBookStart 16
#define OrderIDStart 17
#define CookFinishStart 18
#define InventoryStart 22//库存管理起始位置
#define InventoryStateStart 26//库存管理状态起始位置
//一些固定位置索引
#define FunctionCode 7

#define H_RX_BUF_SIZE		128
#define H_TX_BUF_SIZE      	128

typedef struct
{
	uint8_t RxBuf[H_RX_BUF_SIZE];
	uint8_t RxCount;

	uint8_t TxBuf[H_TX_BUF_SIZE];
	uint8_t TxCount;
	uint16_t MODTCPHead;

	/************************************
	 * 对应的位 0 表示执行失败 1表示执行成功 
	 * 因Modbus常见功能码不超过32所以可以用一个32
	 * 位的数统计应答，比如10H，就是查询Ack的
	 * 10H位是否置1
	***********************************/
	uint32_t Ack;		
	

}MODH_T;

typedef struct
{
	uint16_t reg_addr;
	uint16_t reg_value;
}ModbusReg_T;

typedef struct __ModTCPList
{
	uint8_t *data;//如果需要重传才需要这个数据域，如果不需要重传，只需要将收发匹配那只需要MODTCPHead这个就够了
	uint16_t MODTCPHead;
	uint32_t TimeStamp;//重发用到，记录发送时间，每次重发重新记录，然后没10ms轮训一次队列，看谁要重发了
	struct __ModTCPList *next;
}ModTCPList,*pModTCPList;
typedef struct __ModTCPListHead
{
	pModTCPList pHead;//
	pModTCPList pTail;//
	uint16_t ListLen;
}ModTCPListHead;

extern ModTCPListHead ModbusTCPTxDataHead;
extern MODH_T g_tModH;
extern void ModbusTCP_Cyclic_Communication(void);
extern ModbusReg_T TCPModbusRegArray[30];
extern void MODH_Poll(void);
extern uint8_t MODH_ReadParam_01H(uint16_t _reg, uint16_t _num);
extern uint8_t MODH_ReadParam_02H(uint16_t _reg, uint16_t _num);
extern uint8_t MODH_ReadParam_03H(uint16_t _reg, uint16_t _num);
extern uint8_t MODH_ReadParam_04H(uint16_t _reg, uint16_t _num);
extern uint8_t MODH_WriteParam_05H(uint16_t _reg, uint16_t _value);
extern uint8_t MODH_WriteParam_06H(uint16_t _reg, uint16_t _value);
extern uint8_t MODH_WriteParam_10H(uint16_t _reg, uint8_t _num, uint8_t *_buf);
#endif


