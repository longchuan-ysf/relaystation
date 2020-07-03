/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   TCPModbusNetHandle.c 
作者:   龙川
说明:   TCPModbus主机相关部分____当网络出现较大延时时会出问题

***************************************************************************************************/
#include "TCPModbus_M.h"
#include "TCPModbusNetHandle.h"
#include "com.h"
#include "fifo.h"
#include "fun.h"
#include "EquipmentManage.h"
#include "TCPModbus.h"

#define MESSAGE_TIMEOUT 5000//5s的超时时间
#define TRY_AGAIN_CNT 1//0不尝试再发一次 超时重新尝试再发次数
#define SlaveAddr			0x01
#define ModbusTimeout		5000//5s等不到信号量就退出
static void MODH_AnalyzeApp(uint16_t reg);

static void MODH_Read_01H(uint16_t reg);
static void MODH_Read_02H(uint16_t reg);
static void MODH_Read_03H(uint16_t reg);
static void MODH_Read_04H(uint16_t reg);
static void MODH_Read_05H(uint16_t reg);
static void MODH_Read_06H(uint16_t reg);
static void MODH_Read_10H(uint16_t reg);
MODH_T g_tModH;
ModTCPListHead ModbusTCPTxDataHead;
static uint8_t AnalyzeData[128];

ModbusReg_T TCPModbusRegArray[]=
{
	//电磁炉告警寄存器地址 
	{2000,0},//0
	{2001,0},//1
	{2002,0},//2
	{2003,0},//3
	
	//电机告警寄存器地址 
	{2010,0},//4
	{2011,0},//5
	{2012,0},//6
	{2013,0},//7
	//冷柜电机告警
	{2020,0},//8
	
	//急停
	{2021,0},//9
	
	//炒菜异常寄存器
	{2022,0},//10
	
	//设备状态寄存器
	{2030,0},//11
	{2031,0},//12
	{2032,0},//13
	{2033,0},//14
	{2034,0},//15
	
	//菜谱编号不能初始化成0
	{900,0xff},//16
	//订单ID
	{901,0},//17
	//订单完成寄存器
	{902,0},//18
	{903,0},//19
	{904,0},//20
	{905,0},//21

	//菜品制作数据寄存器
	{8000,0},//22
	{8001,0},//23
	{8002,0},//24
	{8003,0},//25

	//菜品制作状态寄存器
	{8100,0},//22
	{8101,0},//23
	{8102,0},//24
	{8103,0},//25
};
/**
****************************************************************************************
@brief:    CANReTxListClear CAN重传列表清空
@Input：   NULL
@Output：  NULL
@Return    NULL
@Warning:  NULL——重新签到清空——链表过长清空
@note:     龙川2019-9-10
****************************************************************************************
 **/
void ModbusTCPTxListClear(void)
{
	pModTCPList pTemp;//
	if(ModbusTCPTxDataHead.pHead == NULL)
	{
		return;
	}
	while(ModbusTCPTxDataHead.pHead != NULL)
	{
		pTemp = ModbusTCPTxDataHead.pHead->next;
		free(ModbusTCPTxDataHead.pHead->data);//释放数据区
		free(ModbusTCPTxDataHead.pHead);//释放本身
		ModbusTCPTxDataHead.pHead = pTemp;//指向下一个节点
		ModbusTCPTxDataHead.ListLen--;
	}
	ModbusTCPTxDataHead.pHead = NULL;
	ModbusTCPTxDataHead.pTail = NULL;
	ModbusTCPTxDataHead.ListLen = 0;
}
/**
****************************************************************************************
@brief:   CANReTxAddData CAN重传处理
@Input:   Data 数据
		  len数据长度
		  Addr重传地址
@Output:  NULL
@Return   NULL
@Warning: NULL 
@note:    龙川2019-9-10
****************************************************************************************
 **/
void ModbusTCPAddData(const uint8_t *Data,uint16_t len)
{
	struct timeval tv;
	if(Data == NULL)
	{
		return;
	}
	
	pModTCPList pAppdata;//
	uint8_t *pData;
	
	//链表最大长度不超过16
	if(ModbusTCPTxDataHead.ListLen>8)
	{
		//清空链表————————这样可能会出现丢失数据，但是一般超过几个没回复都可以说明掉线了
		dzlog_warn("warning!!!!! List Too long!!!!!!\n");
		EquipmentManage.tcp_modbus.tcp_connect_state = false;
		
		ModbusTCPTxListClear();
		return;
	}
	pData = (uint8_t *)malloc(len);
	if (NULL == pData)
	{
		dzlog_warn("malloc pData :%d bytes failed\n", len);
		return ;
	}

	memcpy(pData,Data,len);

	
	pAppdata = (pModTCPList)malloc(sizeof(ModTCPList));
	if (NULL == pAppdata)
	{
		dzlog_warn("malloc pAppdata:%d bytes failed\n", len);
		return ;
	}
	gettimeofday(&tv,NULL);
	pAppdata->data = pData;
	pAppdata->TimeStamp = tv.tv_sec;
	pAppdata->MODTCPHead = (Data[0]<<8)|Data[1];
	pAppdata->next = NULL;


	if(ModbusTCPTxDataHead.ListLen == 0)//第一个要添加链表头
	{
		ModbusTCPTxDataHead.pHead = pAppdata;
		ModbusTCPTxDataHead.pTail = pAppdata;
	}
	else
	{
		ModbusTCPTxDataHead.pTail->next = pAppdata;
		ModbusTCPTxDataHead.pTail = pAppdata;
	}
	ModbusTCPTxDataHead.ListLen++;
}
/**
****************************************************************************************
@brief:   ModbusTCPRespondCheck 
@Input:   CheckModTCPHead 要检查的数据事物处理标识
@Output:  NULL
@Return   0：在链表中找到匹配项
		  非0：在链表中找不到匹配项
@Warning: NULL 
@note:    龙川2019-7-12
****************************************************************************************
 **/
int8_t ModbusTCPRespondCheck(uint16_t CheckModTCPHead,uint16_t *reg)
{
	int8_t match;
	uint16_t MODTCPHead;
	uint8_t *pdata;
	pModTCPList pTemp,pTempHead;//pTemp为匹配的节点，pTempHead为匹配节点的上一级节点
	if(ModbusTCPTxDataHead.ListLen == 0)
	{
		dzlog_debug("err: List length = 0\r\n");
		return -1;
	}
	match = -1;
	pTempHead = pTemp = ModbusTCPTxDataHead.pHead;
	while(pTemp != NULL)
	{
		MODTCPHead = pTemp->MODTCPHead;
		
		if(MODTCPHead == CheckModTCPHead)//找到匹配项	
		{
			/*************先将匹配标识置0***********************/
			match = 0;
			/*************获取寄存器地址***********************/
			pdata = pTemp->data;
			*reg = (pdata[8]<<8)|pdata[9];
			/*************然后将匹配项从链表中删除***********************/

			//如果删除的是链表头
			if(pTemp == ModbusTCPTxDataHead.pHead)
			{
				//头先指向下一个节点
				ModbusTCPTxDataHead.pHead = pTempHead->next;
				free(pTemp->data);//释放数据区
				free(pTemp);//释放本身

			}
			else if(pTemp == ModbusTCPTxDataHead.pTail)//尾节点
			{
				//尾节点指向上一级
				pTempHead->next = NULL;//尾节点不能指向下一级了
				ModbusTCPTxDataHead.pTail = pTempHead;
				free(pTemp->data);//释放数据区
				free(pTemp);//释放本身
			}
			else
			{
				//将上一级指向下一级的next
				pTempHead->next = pTemp->next;
				free(pTemp->data);//释放数据区
				free(pTemp);//释放本身
			}
			if(ModbusTCPTxDataHead.ListLen)
			{
				ModbusTCPTxDataHead.ListLen--;
			}	
			break;
		}
		//保存上一级节点
		pTempHead = pTemp;
		//寻找节点指向下一级
		pTemp = pTempHead->next;	
	}
	return match;
}
/*
*********************************************************************************************************
*	函 数 名: MODH_SendInit
*	功能说明: 初始化
*	形    参: _addr：从站地址		  
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_SendInit(uint8_t _addr)
{
	if(EquipmentManage.tcp_modbus.tcp_connect_state == false)
		return;
	
	//g_tModH.MODTCPHead = HAL_GetTick()&0xffff;
	//不能根据时间来可能同一秒有很多数据发送
	g_tModH.MODTCPHead = g_tModH.MODTCPHead + 1;
	g_tModH.TxCount = 0;
	//事务处理标志
	g_tModH.TxBuf[g_tModH.TxCount++] = (g_tModH.MODTCPHead>>8)&0xff;
	g_tModH.TxBuf[g_tModH.TxCount++] = g_tModH.MODTCPHead&0xff;
	//协议标识符——默认0
	g_tModH.TxBuf[g_tModH.TxCount++] = 0;
	g_tModH.TxBuf[g_tModH.TxCount++] = 0;
	//长度域，默认先用0占位
	g_tModH.TxBuf[g_tModH.TxCount++] = 0;
	g_tModH.TxBuf[g_tModH.TxCount++] = 0;
	//单元标识符也就是从站地址
	g_tModH.TxBuf[g_tModH.TxCount++] = _addr;		/* 从站地址 */
}
/*
*********************************************************************************************************
*	函 数 名: MODH_SendPacket
*	功能说明: 发送数据包 COM1口
*	形    参: buf : 数据缓冲区
*			  len : 数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_SendPacket(uint8_t *buf, uint16_t len)
{
	//重新计算长度域，长度域不包含事务处理标志、协议标识和长度域字节的长度
	g_tModH.TxBuf[4] = ((len-6)>>8)&0xff;
	g_tModH.TxBuf[5] = (len-6)&0xff;
	
	if(TCPModbus_Client_Send_Data(buf,len))
	{
		dzlog_warn("Disconnect TCPModbus\n");

		EquipmentManage.tcp_modbus.tcp_connect_state = false;//发送出错重连
		return ;
	}
	dzlog_debug("TCPModbus TX \n");
	printf_Hexbuffer(buf,len);		


	//加入链表，方便后面数据接收时查找对应的发送命令——就算是一收一发也必须用链表保存
	//因为有的机器是有缓存，有的没有如PLC，但是两种都要兼顾
	ModbusTCPAddData(buf,len);
	//清零应答标志位
	if(buf[FunctionCode]<32)
	{
		g_tModH.Ack &= ~(0x01<<buf[FunctionCode]);
	}
	//usleep(6000);
}

/*
*********************************************************************************************************
*	函 数 名: MODH_AnalyzeApp
*	功能说明: 分析应用层协议。处理应答。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_AnalyzeApp(uint16_t reg)
{	
	switch (g_tModH.RxBuf[FunctionCode])			/* 第8个字节 功能码 */
	{
		case 0x01:	/* 读取线圈状态 */
			MODH_Read_01H(reg);
			break;

		case 0x02:	/* 读取输入状态 */
			MODH_Read_02H(reg);
			break;

		case 0x03:	/* 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值 */
			MODH_Read_03H(reg);
			break;

		case 0x04:	/* 读取输入寄存器 */
			MODH_Read_04H(reg);
			break;

		case 0x05:	/* 强制单线圈 */
			MODH_Read_05H(reg);
			break;

		case 0x06:	/* 写单个寄存器 */
			MODH_Read_06H(reg);
			break;		

		case 0x10:	/* 写多个寄存器 */
			MODH_Read_10H(reg);
			break;
		
		default:
			break;
	}
	//设立接受完成标志位
	if(g_tModH.RxBuf[FunctionCode]<32)
	{
		g_tModH.Ack |= (0x01<<g_tModH.RxBuf[FunctionCode]);
	}
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Poll
*	功能说明: 接收控制器指令. 1ms 响应时间。
*	形    参: 无
*	返 回 值: 0 表示无数据 1表示收到正确命令
*********************************************************************************************************
*/
void MODH_Poll(void)
{	
	uint16_t ModbusHead;
	uint16_t RxLen,i,reg;
	RxLen = fifo_data_length(&TCPModbusFIFO.rxFifo);
	if (RxLen < 7)
	{
		return;
	}
	fifo_out(&TCPModbusFIFO.rxFifo,AnalyzeData,RxLen);
	dzlog_debug("tcp modbus rx:\r\n");
	printf_Hexbuffer(AnalyzeData,RxLen);
	ModbusHead = (AnalyzeData[0]<<8)|AnalyzeData[1];
	if(ModbusTCPRespondCheck(ModbusHead,&reg))
	{
		dzlog_warn("cann't find match Modbus Head %d\r\n",ModbusHead);
		return;
	}
	memcpy(g_tModH.RxBuf,AnalyzeData,RxLen);
	g_tModH.RxCount = RxLen;
	/* 分析应用层协议 */
	MODH_AnalyzeApp(reg);
	//因为PLC是一个PLC多个模块所以默认和PLC通信成功就是4口锅都正常上线
	for(i=0; i<TotalCookerNum; i++)
		EquipmentManage.CookerManage[i].ConnectState = 1;
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send01H
*	功能说明: 发送01H指令，查询1个或多个保持寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send01H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x01;		/* 功能码 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;	/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;		/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8;	/* 寄存器个数 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num;		/* 寄存器个数 低字节 */
	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);		/* 发送数据 */	
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send02H
*	功能说明: 发送02H指令，读离散输入寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send02H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x02;		/* 功能码 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;	/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;		/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8;	/* 寄存器个数 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num;		/* 寄存器个数 低字节 */
	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);	/* 发送数据*/
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send03H
*	功能说明: 发送03H指令，查询1个或多个保持寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send03H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x03;		/* 功能码 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;	/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;		/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8;	/* 寄存器个数 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num;		/* 寄存器个数 低字节 */
	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);		/* 发送数据 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send04H
*	功能说明: 发送04H指令，读输入寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send04H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x04;		/* 功能码 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;	/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;		/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8;	/* 寄存器个数 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num;		/* 寄存器个数 低字节 */
	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);		/* 发送数据*/
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send05H
*	功能说明: 发送05H指令，写强置单线圈
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _value : 寄存器值,2字节
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send05H(uint8_t _addr, uint16_t _reg, uint16_t _value)
{
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x05;			/* 功能码 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;		/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;			/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _value >> 8;		/* 寄存器值 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _value;			/* 寄存器值 低字节 */
	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);		/* 发送数据 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send06H
*	功能说明: 发送06H指令，写1个保持寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _value : 寄存器值,2字节
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send06H(uint8_t _addr, uint16_t _reg, uint16_t _value)
{
	uint8_t i;
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x06;			/* 功能码 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;		/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;			/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _value >> 8;		/* 寄存器值 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _value;			/* 寄存器值 低字节 */
	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);		/* 发送数据*/
	
	//本地寄存器优先修改——其实这里最好是在10H的回复中修改，但是我为了偷懒就在这里修改了
	for(i=0; i<sizeof(TCPModbusRegArray)/sizeof(ModbusReg_T); i++)
	{
		if(_reg == TCPModbusRegArray[i].reg_addr)
		{
			
			TCPModbusRegArray[i].reg_value = _value;
			dzlog_debug("set reg:%d,value:%d\r\n",TCPModbusRegArray[i].reg_addr,TCPModbusRegArray[i].reg_value);
			break;
		}
	}

	
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send10H
*	功能说明: 发送10H指令，连续写多个保持寄存器. 最多一次支持23个寄存器。
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数n (每个寄存器2个字节) 值域
*			  _buf : n个寄存器的数据。长度 = 2 * n
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send10H(uint8_t _addr, uint16_t _reg, uint8_t _num, uint8_t *_buf)
{
	uint16_t i;
	uint8_t regnum;
	uint16_t reg;
	uint8_t *p;
	MODH_SendInit(_addr);
	g_tModH.TxBuf[g_tModH.TxCount++] = 0x10;		/* 从站地址 */	
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;	/* 寄存器编号 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _reg;		/* 寄存器编号 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8;	/* 寄存器个数 高字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = _num;		/* 寄存器个数 低字节 */
	g_tModH.TxBuf[g_tModH.TxCount++] = 2 * _num;	/* 数据字节数 */
	
	for (i = 0; i < 2 * _num; i++)
	{
		if (g_tModH.TxCount > H_RX_BUF_SIZE - 3)
		{
			return;		/* 数据超过缓冲区超度，直接丢弃不发送 */
		}
		g_tModH.TxBuf[g_tModH.TxCount++] = _buf[i];		/* 后面的数据长度 */
		
		
		
	}
	//本地寄存器优先修改——其实这里最好是在10H的回复中修改，但是我为了偷懒就在这里修改了
	regnum = _num;
	p = _buf;
	reg = _reg;
	while(regnum)
	{
		for(i=0; i<sizeof(TCPModbusRegArray)/sizeof(ModbusReg_T); i++)
		{
			if(reg == TCPModbusRegArray[i].reg_addr)
			{
				
				TCPModbusRegArray[i].reg_value = BEBufToUint16(p);
				p += 2;
				dzlog_debug("set reg:%d,value:%d\r\n",TCPModbusRegArray[i].reg_addr,TCPModbusRegArray[i].reg_value);
				break;
			}
		}
		reg++;
		regnum--;
	}	
	MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);	/* 发送数据*/
	
}




/*
*********************************************************************************************************
*	函 数 名: MODH_Read_01H
*	功能说明: 分析01H指令的应答数据_协议中没用到01指令就不详细解析了
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_01H(uint16_t reg)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_02H
*	功能说明: 分析02H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_02H(uint16_t reg)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_04H
*	功能说明: 分析04H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_04H(uint16_t reg)
{
	
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_05H
*	功能说明: 分析05H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_05H(uint16_t reg)
{

}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_06H
*	功能说明: 分析06H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_06H(uint16_t reg)
{
	
}


/*
*********************************************************************************************************
*	函 数 名: MODH_Read_03H
*	功能说明: 分析03H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_03H(uint16_t reg)
{
	uint8_t bytes,i;
	uint8_t *p;
	
	if (g_tModH.RxCount > 0)
	{
		bytes = g_tModH.RxBuf[6+2];	/* 数据长度 字节数 */		
		p = &g_tModH.RxBuf[6+3];
		while(bytes)
		{
			for(i=0; i<sizeof(TCPModbusRegArray)/sizeof(ModbusReg_T); i++)
			{
				if(reg == TCPModbusRegArray[i].reg_addr)
				{
					
					TCPModbusRegArray[i].reg_value = BEBufToUint16(p);
					p += 2;
					dzlog_debug("read reg:%d,value:%d\r\n",TCPModbusRegArray[i].reg_addr,TCPModbusRegArray[i].reg_value);
					break;
				}
			}
			bytes-=2;
			reg ++;//因为连续访问寄存器的地址是递增的
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_10H
*	功能说明: 分析10H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_10H(uint16_t reg)
{

}
/*
*********************************************************************************************************
*	函 数 名: MODH_Check_Responed检测是否回复
*	功能说明: Funcd 功能码
*	形    参: 无
*	返 回 值: 0 表示成功。1 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
static uint8_t MODH_Check_Responed(uint8_t Funcd)
{
	uint8_t Ack,ret,try_again;
	uint16_t timeout;
	timeout = 0;
	ret = 0;
	//debugf("%s\r\n",__func__);
	if(TRY_AGAIN_CNT)
		try_again = TRY_AGAIN_CNT;
	else
	{
		try_again = 0;
	}
	
	do
	{
		usleep(1000);
		MODH_Poll();	
		Ack = (g_tModH.Ack>>Funcd)&0x01;
		timeout++;
		if(timeout > MESSAGE_TIMEOUT)
		{
			//尝试重发次数用完直接退出
			if(!try_again)
			{
				ret = 1;
				EquipmentManage.tcp_modbus.tcp_connect_state = false;
				//先清掉之前链表的数据_因为现在必须等数据回复——PLC决定的所以这里简单粗暴直接清掉整个链表
				ModbusTCPTxListClear();
				dzlog_debug("timeout");
				break;
			}
			else
			{
				timeout = 0;
				if(try_again>0)
					try_again--;
				MODH_SendPacket(g_tModH.TxBuf,g_tModH.TxCount);		/* 发送数据 */	
			}
			
		}
	} while (!Ack);	
	return ret;
}
/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_01H
*	功能说明: 单个参数. 通过发送01H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 0 表示成功。1 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_01H(uint16_t _reg, uint16_t _num)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send01H (SlaveAddr, _reg, _num);		  /* 发送命令 */
	ret = MODH_Check_Responed(0x01);
	//debugf("%s state = %d",);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	return ret;
}

/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_02H
*	功能说明: 单个参数. 通过发送02H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_02H(uint16_t _reg, uint16_t _num)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send02H (SlaveAddr, _reg, _num);
	
	ret = MODH_Check_Responed(0x02);
	//debugf("%s state = %d",);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	return ret;
}
/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_03H
*	功能说明: 单个参数. 通过发送03H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_03H(uint16_t _reg, uint16_t _num)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send03H(SlaveAddr, _reg, _num);
	ret = MODH_Check_Responed(0x03);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	return ret;
}


/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_04H
*	功能说明: 单个参数. 通过发送04H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_04H(uint16_t _reg, uint16_t _num)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send04H (SlaveAddr, _reg, _num);
	ret = MODH_Check_Responed(0x04);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	//debugf("%s state = %d",);
	return ret;
}
/*
*********************************************************************************************************
*	函 数 名: MODH_WriteParam_05H
*	功能说明: 单个参数. 通过发送05H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_WriteParam_05H(uint16_t _reg, uint16_t _value)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send05H (SlaveAddr, _reg, _value);
	ret = MODH_Check_Responed(0x05);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	//debugf("%s state = %d",);
	return ret;
}

/*
*********************************************************************************************************
*	函 数 名: MODH_WriteParam_06H
*	功能说明: 单个参数. 通过发送06H指令实现，发送之后，等待从机应答。循环NUM次写命令
*	形    参: 无
*	返 回 值: 1表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_WriteParam_06H(uint16_t _reg, uint16_t _value)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send06H (SlaveAddr, _reg, _value);	
	ret = MODH_Check_Responed(0x06);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	//debugf("%s state = %d",);
	return ret;
}

/*
*********************************************************************************************************
*	函 数 名: MODH_WriteParam_10H
*	功能说明: 单个参数. 通过发送10H指令实现，发送之后，等待从机应答。循环NUM次写命令
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/ 
uint8_t MODH_WriteParam_10H(uint16_t _reg, uint8_t _num, uint8_t *_buf)
{
	uint8_t ret;
	//发送前锁住发送缓存区
	pthread_mutex_lock(&TCPModbus_mutex);
	MODH_Send10H(SlaveAddr, _reg, _num, _buf);	
	ret = MODH_Check_Responed(0x10);
	//发送完成后解锁发送缓存区
	pthread_mutex_unlock(&TCPModbus_mutex);
	//debugf("%s state = %d",);
	return ret;
}

void ModbusTCP_Cyclic_Communication()
{
	uint8_t i;
	if(TCPModbus_1secFlag)
	{
		//5s读一次设备状态
		if(TCPModbus_1sec%5 == 0)
		{	
			MODH_ReadParam_03H(2030,5);	
			for(i=0;i<TotalCookerNum;i++)
			{
				EquipmentManage.CookerManage[i].EquipmentState = (EquipmentStateEnum)TCPModbusRegArray[CookerState].reg_value;
				//当前PLC没有做到很精确的告警位置上报，所以默认都是产生急停
				if(EquipmentManage.CookerManage[i].EquipmentState == Warnning)
				{
					EquipmentManage.CookerManage[i].AlarmRecord[0] = 0x01;
				}
				else//告警被清除了，就要清除告警位信息
				{
					EquipmentManage.CookerManage[i].AlarmRecord[0] = 0x00;
				}
				
			}
			EquipmentManage.RefrigertorManage.EquipmentState = (EquipmentStateEnum)TCPModbusRegArray[RefrigertorState].reg_value;
			if(EquipmentManage.RefrigertorManage.EquipmentState == Warnning)
			{
				EquipmentManage.RefrigertorManage.AlarmRecord[0] = 0x01;
			}
			else//告警被清除了，就要清除告警位信息
			{
				EquipmentManage.RefrigertorManage.AlarmRecord[0] = 0x00;
			}
	
			
			//因为没有查询具体告警内容，因为PLC没有做
		}
		//查询菜谱ID是否为0——每s查询一次
//		
		// if(TCPModbus_1sec%3 == 0)
		// {
		// 	MODH_ReadParam_03H(TCPModbusRegArray[CookBookStart].reg_addr,1);
		// }
		// if(TCPModbus_1sec%5 == 0)
		// {
		// 	MODH_ReadParam_03H(TCPModbusRegArray[CookFinishStart].reg_addr,4);
		// }
		if(TCPModbus_1sec%15 == 0)
		{
			MODH_ReadParam_03H(TCPModbusRegArray[InventoryStart].reg_addr,4);
			MODH_ReadParam_03H(TCPModbusRegArray[InventoryStateStart].reg_addr,4);
		}
				
	}
}
