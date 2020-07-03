/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   CookerFlashRecord.h 
作者:   龙川
说明:   LED灯控制
***************************************************************************************************/

#ifndef __RECORD_H
#define __RECORD_H
#include "com.h"
#include "EquipmentManage.h"

#define RECORD_SYSPARA 		"./record/record_syspara.xlsx"
#define RECORD_ERR 			"./record/record_err.xlsx"
#define RECORD_ORDER 		"./record/record_order.xlsx"
#define RECORD_FOOD 		"./record/record_FoodMaterial.xlsx"

#define MAX_REFRIGERTOR_ROW 16//定义冷柜能存储N行
#define MAX_REFRIGERTOR_COLUMN 16//定义冷柜能存储M列
//系统参数永远是字符串，字符串长度不超过30字节
typedef struct
{
	char name[20];
	char parament[32];
}systempara;
//如有新的参数请往后加
typedef struct
{
	systempara EquipmentSN;
	systempara serverip;
	systempara serverport;
	systempara modbusip;
	systempara modbusport;
}SaveParaStruct;
typedef struct
{
	uint8_t num;//该通道该菜品剩余个数
	uint8_t SN[16];//菜品编号
}FoodMaterial;

extern FoodMaterial ReFoodMaterial[MAX_REFRIGERTOR_ROW][MAX_REFRIGERTOR_COLUMN];//最多N×M个通道
extern SaveParaStruct SavePara;
//系统参数
extern void SysParaPowerOnInit(void);
//冷柜库存
extern void RefrigertorPowerOnInit(void);
extern void RefrigertorWrite(void);
//订单保存
extern void CookerFlashRecordInit(void);
extern void SaveOrderPowerOnInit(void);
extern void SaveOrderRecordAdd(OrderInfoStruct * pOrderInfoStruct);
extern void SaveOrderRecordClean(uint8_t * OrderSn);
extern void SaveOrderRecordCleanAll(void);

#endif


