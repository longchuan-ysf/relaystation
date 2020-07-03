/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   CookerManager.c 
作者:   龙川
说明:   机器管理管理源文件
***************************************************************************************************/
#include "com.h"
#include "EquipmentManage.h"
#include "TCPMessageHandle.h"
#include "TCPModbus_M.h"
#include "Record.h"

EquipmentManangeStruct EquipmentManage;
//uint32_t background;
static void printf_syspara(void)
{
	
	dzlog_debug("EquipmentSN:%s\r\n",EquipmentManage.EquipmentSn);
	dzlog_debug("serverip:%s\r\n",EquipmentManage.tcp_background.serverip);
	dzlog_debug("serverport:%s\r\n",EquipmentManage.tcp_background.serverport);
	dzlog_debug("modbusip:%s\r\n",EquipmentManage.tcp_modbus.serverip);
	dzlog_debug("modbusport:%s\r\n",EquipmentManage.tcp_modbus.serverport);

}
  /**
 ****************************************************************************************
 @brief:	OrderManagerInit 订单管理相关初始化
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-7-6
 ****************************************************************************************
  **/
void EquipmentManagerInit(void)
{
	printf("%s\r\n",__func__);
	memset(&EquipmentManage,0,sizeof(EquipmentManangeStruct));
	//上电要从Flash读取一些参数这里先不做
	

	EquipmentManage.NetStackOverflow = 0;


	EquipmentManage.Timestamp = 1585791117;//初始化的时间——实际要与后台对时具备掉电保存

	
	EquipmentManage.OrderDeal.ValidLen = 0;
	EquipmentManage.OrderDeal.TotalLen = QueueTotalLen;
	EquipmentManage.OrderDeal.UsedLen = 0;
	EquipmentManage.OrderDeal.FreeLen = EquipmentManage.OrderDeal.ValidLen - EquipmentManage.OrderDeal.UsedLen;
	
	memcpy(EquipmentManage.EquipmentSn,SavePara.EquipmentSN.parament,strlen(SavePara.EquipmentSN.parament));
	EquipmentManage.RunMode = RUN_MODE_DEBUG; //RUN_MODE_NORMAL;


	memcpy(EquipmentManage.tcp_background.serverip,SavePara.serverip.parament,strlen(SavePara.serverip.parament));
	memcpy(EquipmentManage.tcp_background.serverport,SavePara.serverport.parament,strlen(SavePara.serverport.parament));
	
	// sprintf((char *)EquipmentManage.tcp_background.serverip,"%s","120.234.134.139");
	// sprintf((char *)EquipmentManage.tcp_background.serverport,"%s","18899");
	// sprintf((char *)EquipmentManage.tcp_background.serverip,"%s","192.168.2.53");
	// sprintf((char *)EquipmentManage.tcp_background.serverport,"%s","8888");
	memcpy(EquipmentManage.tcp_modbus.serverip,SavePara.modbusip.parament,strlen(SavePara.modbusip.parament));
	memcpy(EquipmentManage.tcp_modbus.serverport,SavePara.modbusport.parament,strlen(SavePara.modbusport.parament));

	// sprintf((char *)EquipmentManage.tcp_modbus.serverip,"%s","192.168.1.250");
	// sprintf((char *)EquipmentManage.tcp_modbus.serverport,"%s","502");
	printf_syspara();
	EquipmentManage.HeartbeatFre = 7;//心跳包周期7s不能太长，太长会导致read函数返回错误——阻塞方式读数据     
	EquipmentManage.HeartbeatTimeout = 3;//心跳包超时次数
}
 /**
****************************************************************************************
@brief:    ValidCookerCheck 检测有效的锅
@Input：     NULL
@Output：	NULL
@Return    NULL
@Warning:  NULL
@note:	   龙川2019-07-08
****************************************************************************************
 **/
void ValidCookerCheck()
{
	uint8_t i,ValidLen;
	ValidLen = 0;
	for(i=0; i<TotalCookerNum; i++)
	{
		if(EquipmentManage.RunMode == RUN_MODE_DEBUG)
		{
			EquipmentManage.CookerManage[i].ConnectState = 1;
		}
		else
		{
			if(EquipmentManage.CookerManage[i].ConnectState == 0)
			{
				continue;             
			}
		}

		if(EquipmentManage.CookerManage[i].EquipmentState != Warnning)
		{
			if(ValidLen<TotalCookerNum)
			ValidLen++;
		}
	}
	
	if(EquipmentManage.RunMode == RUN_MODE_DEBUG_Refrigertor)
	{
		ValidLen = 1;
	}
	else if(EquipmentManage.RunMode == RUN_MODE_DEBUG)
	{
		ValidLen = TotalCookerNum;
	}
	else if(EquipmentManage.RunMode == RUN_MODE_NORMAL)
	{
		if(EquipmentManage.RefrigertorManage.ConnectState == 0)
		{
			ValidLen = 0;
		}
	}
	if(ValidLen)
	{
		EquipmentManage.OrderDeal.ValidLen = ValidLen*2;//有效长度等于在线机器数量*2
		EquipmentManage.OrderDeal.FreeLen = EquipmentManage.OrderDeal.ValidLen - EquipmentManage.OrderDeal.UsedLen;;
	}
	else
	{
		//当机器正在炒菜UsedLen不为0，然后机器突然全部掉线——之前手工模拟过，此时按上面的计算就会算出负数然后变成一个很大的正数
		EquipmentManage.OrderDeal.FreeLen = 0;
	}
	
}



  /**
  ****************************************************************************************
  @brief:	CalculateTaskTime 计算订单完成时间
  @Input：   pOrderInfol 哪个机器的控制结构体
  @Output：  NULL
  @Return:	计算结果
  @Warning:  NULL  
  @note:   龙川2019-7-08
  ****************************************************************************************
  **/
  uint32_t  CalculateTaskTime(OrderInfoStruct *pOrderInfo)
  {
	uint32_t Time;
	uint8_t i;
	if(pOrderInfo == NULL)
	{
		return 0;
	}
	if(EquipmentManage.RunMode == RUN_MODE_DEBUG)
	{
		return 60*3*1000;//3min
	}
	Time = 0;
	for(i=0; i<pOrderInfo->TotalStep; i++)
	{
		//只计算翻炒时间和加油变温时间
		if((pOrderInfo->CookBook[i].OperationCode == Enum_StirFry)\
			||(pOrderInfo->CookBook[i].OperationCode == Enum_AddOil)\
			||(pOrderInfo->CookBook[i].OperationCode == Enum_ChangeTemp)\
			||(pOrderInfo->CookBook[i].OperationCode == Enum_AddWater)\
			||(pOrderInfo->CookBook[i].OperationCode == Enum_AddSauce1)\
			||(pOrderInfo->CookBook[i].OperationCode == Enum_AddSauce2)\
			||(pOrderInfo->CookBook[i].OperationCode == Enum_AddSauce2))
			Time += (pOrderInfo->CookBook[i].OperationTime[0]+(pOrderInfo->CookBook[i].OperationTime[1]<<8));
	}
	return Time + 50000;//时间，基础时间+30s是为了防止出现由于有的机器需要先升锅等操作导致会出现一直未1的时间
  }

    /**
  ****************************************************************************************
  @brief:	CheckOrderFinish 检测现在制作中的订单是否制作完成
  @Input：  NULL
  @Output： NULL
  @Return:	计算结果
  @Warning: NULL  
  @note:    龙川2019-7-08
  ****************************************************************************************
  **/
 void CheckOrderFinish()
 {
	uint8_t i,j;
	uint32_t  TaskSn;
	struct  timeval    tv;
	//订单队列为空，就没有订单管理
	if(!EquipmentManage.OrderDeal.UsedLen)
	{
		return;
	}
	gettimeofday(&tv,NULL);
	//每3秒查询一次
	//if(tv%3 == 0)
	{
		//将4个寄存器的值获取，然后和现有的订单任务比较，如果匹配就算制作完成
		for(i=0; i<TotalCookerNum; i++)
		{
			//如果寄存器有值
			if(TCPModbusRegArray[CookFinishStart+i].reg_value)
			{
				//循环找出现有订单
				for(j=0; j<EquipmentManage.OrderDeal.UsedLen; j++)
				{
					TaskSn = EquipmentManage.OrderDeal.OrderInfo[j].TaskSn[0]+(EquipmentManage.OrderDeal.OrderInfo[j].TaskSn[1]<<8);
					if(TaskSn == TCPModbusRegArray[CookFinishStart+i].reg_value)
					{
						EquipmentManage.OrderDeal.OrderInfo[j].OrderState = CookFinish;
						TCPModbusRegArray[CookFinishStart+i].reg_value = 0;
						MODH_WriteParam_06H(TCPModbusRegArray[CookFinishStart+i].reg_addr,TCPModbusRegArray[CookFinishStart+i].reg_value);
						break;
					}
				}
			}
		}
	}
	 
 }
/**
  ****************************************************************************************
  @brief:	InventoryManager 本地下单库存管理
  @Input：  NULL
  @Output： NULL
  @Return:	NULL
  @Warning: NULL  
  @note:    龙川2020-5-21
  ****************************************************************************************
  **/
void InventoryManager(void)
{
	uint8_t i,find,timeout;
	uint8_t row,column;
	uint32_t sn;
	struct  timeval    tv;
	gettimeofday(&tv,NULL);
	//每3秒查询一次
	//if(tv.tv_sec%3 == 0)
	{
		//将4个寄存器的值获取，获取消耗库存
		for(i=0; i<4; i++)
		{
			//如果寄存器有值
			if(TCPModbusRegArray[InventoryStart+i].reg_value)
			{
				find = 0;
				//看下已有的库存记录中是否有记录
				for(row=0; row<MAX_REFRIGERTOR_ROW; row++)
				{
					for(column=0; column<MAX_REFRIGERTOR_COLUMN; column++)
					{
						sn = atoi((char *)ReFoodMaterial[row][column].SN);
					
						if(TCPModbusRegArray[InventoryStart+i].reg_value == sn)
						{
							find = 1;
							CMD20C_Handle(row+1,column+1,TCPModbusRegArray[InventoryStart+i].reg_value);
							break;
						}
					}
					//如果已经找到匹配的记录，跳出循环
					if(find)
						break;
				}
				//如果没有找到
				if(!find)
				{
					CMD20C_Handle(0xff,0xff,TCPModbusRegArray[InventoryStart+i].reg_value);
				}
				timeout = 0;
				do
				{
					//清空寄存器
					TCPModbusRegArray[InventoryStateStart+i].reg_value = 1;
					MODH_WriteParam_06H(TCPModbusRegArray[InventoryStateStart+i].reg_addr,\
										TCPModbusRegArray[InventoryStateStart+i].reg_value);
					sleep(2);
					MODH_ReadParam_03H(TCPModbusRegArray[InventoryStart+i].reg_addr,1);
					timeout++;
					if(timeout>15)
					{
						break;
						dzlog_debug("timeout!\r\n");
					}
					dzlog_debug("------reg %d ----value %d\r\n",\
					TCPModbusRegArray[InventoryStart+i].reg_addr,\
					TCPModbusRegArray[InventoryStart+i].reg_value);
				} while (TCPModbusRegArray[InventoryStart+i].reg_value);		
			}
		}
	}

}
  /**
 ****************************************************************************************
 @brief:	OrderManager 订单处理
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-07-08
 ****************************************************************************************
  **/
void OrderManager(void)
{
	//static uint32_t Time[QueueTotalLen];
	uint8_t i,j;
	//uint8_t cooker;
	uint32_t  MenuSn,TaskSn;
	uint8_t ModbusData[16];

	//订单队列为空，就没有订单管理
	if(!EquipmentManage.OrderDeal.UsedLen)
	{
		return;
	}

	//轮询完所有的正在用的
	for(i=0; i<EquipmentManage.OrderDeal.UsedLen; i++)
	{
		//根据订单状态决定订单处理
		
		switch(EquipmentManage.OrderDeal.OrderInfo[i].OrderState)
		{
			case Wait:
			{
				 if(EquipmentManage.RunMode != RUN_MODE_DEBUG)
				 {
					//PLC菜谱寄存器为0，下订单给PLC
					if(!TCPModbusRegArray[CookBookStart].reg_value)
					{
						//拼凑出菜谱编号
						MenuSn = atoi((char *)EquipmentManage.OrderDeal.OrderInfo[i].MenuSn);
						ModbusData[0] = (MenuSn>>8)&0xff;
						ModbusData[1] =  MenuSn&0xff;
						TaskSn = EquipmentManage.OrderDeal.OrderInfo[i].TaskSn[0]+(EquipmentManage.OrderDeal.OrderInfo[i].TaskSn[1]<<8);
						ModbusData[2] = (TaskSn>>8)&0xff;
						ModbusData[3] = TaskSn&0xff;
						dzlog_debug("\r\nMenuSn = %d,taskSn = %d!!!!!!!!!!!!!!!!\r\n",MenuSn,TaskSn);
						MODH_WriteParam_10H(TCPModbusRegArray[CookBookStart].reg_addr,2,ModbusData);
						
						EquipmentManage.OrderDeal.OrderInfo[i].OrderState = Cooking;
						EquipmentManage.OrderDeal.OrderInfo[i].TimeOutCnt = 0;
	
					}
				}
				else
				{
					EquipmentManage.OrderDeal.OrderInfo[i].OrderState = Cooking;
				}
			}
			break;
			case Prepare:
			{
				
					
			}
			break;
			case PrepareFinish:
			{
									
			}
			break;
			case Cooking:
			{
				
				//每个订单计算耗时约10min，超过10min还没上报，就算超时
				EquipmentManage.OrderDeal.OrderInfo[i].TimeOutCnt++;
				if(EquipmentManage.RunMode != RUN_MODE_DEBUG)
				{
					if(EquipmentManage.OrderDeal.OrderInfo[i].TimeOutCnt>3*60*1000)
					{
						EquipmentManage.OrderDeal.OrderInfo[i].TimeOutCnt = 0;
						//原本是Abnormal的但是PLC那边炒完菜3 4号锅没有上报，所以这边自动认为PLC做完了——5分钟时间
						EquipmentManage.OrderDeal.OrderInfo[i].OrderState = CookFinish;
					}
				}
				else
				{
					if(EquipmentManage.OrderDeal.OrderInfo[i].TimeOutCnt>60*3*1000)
					{
						EquipmentManage.OrderDeal.OrderInfo[i].OrderState = CookFinish;
					}
				}

			}
			break;
			case CookFinish:
			{
				//从制作数组中移出到完成打包数组中
				memcpy(&EquipmentManage.OrderDeal.FinishOrderInfo[EquipmentManage.OrderDeal.FinishOrderCnt],\
						&EquipmentManage.OrderDeal.OrderInfo[i],sizeof(OrderInfoStruct));			
				//整理数组
				for(j=i; j<EquipmentManage.OrderDeal.UsedLen-1; j++)
				{
					memcpy(&EquipmentManage.OrderDeal.OrderInfo[j],\
							&EquipmentManage.OrderDeal.OrderInfo[j+1],sizeof(OrderInfoStruct));
							
				}
				//将最后一个释放
				memset(&EquipmentManage.OrderDeal.OrderInfo[EquipmentManage.OrderDeal.UsedLen-1],0,sizeof(OrderInfoStruct));
				EquipmentManage.OrderDeal.UsedLen = EquipmentManage.OrderDeal.UsedLen-1;
				
				//记录订单状态转换超时用的
				EquipmentManage.OrderDeal.FinishOrderInfo[EquipmentManage.OrderDeal.FinishOrderCnt].TimeOutCnt = 0;
				
				if(EquipmentManage.OrderDeal.FinishOrderCnt < Max_FinishOrder)
				{
					EquipmentManage.OrderDeal.FinishOrderCnt++;
				}
					
				
			}
			break;
			//下面这两状态不会进来，应为炒完之后会移到完成结构体数组中——这个for循环是判断制作数组的
			case Packaging:
			{
				
			}
			break;
			case PackageFinish:
			{
				
			}
			break;
			case Abnormal:
			{
				memcpy(&EquipmentManage.OrderDeal.ErrOrderInfo[EquipmentManage.OrderDeal.ErrOrderCnt],\
							&EquipmentManage.OrderDeal.OrderInfo[i],sizeof(OrderInfoStruct));

				
				if(EquipmentManage.OrderDeal.ErrOrderCnt<Max_ErrOrder)//数组溢出了就顶掉第一个入数组的
				{
					EquipmentManage.OrderDeal.ErrOrderCnt++;
				}
				//移出数组
				for(j=i; j<EquipmentManage.OrderDeal.UsedLen-1; j++)
				{
					memcpy(&EquipmentManage.OrderDeal.OrderInfo[j],\
							&EquipmentManage.OrderDeal.OrderInfo[j+1],sizeof(OrderInfoStruct));
							
				}
				//将最后一个释放
				memset(&EquipmentManage.OrderDeal.OrderInfo[EquipmentManage.OrderDeal.UsedLen-1],0,sizeof(OrderInfoStruct));
				EquipmentManage.OrderDeal.UsedLen = EquipmentManage.OrderDeal.UsedLen-1;

			}
			break;
			
			default:
				break;
		}
	}

}
  /**
 ****************************************************************************************
 @brief:	FinishOrderHandle 完成订单处理处理
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-10-16
 ****************************************************************************************
  **/
void FinishOrderHandle()
{
	
	uint8_t i;
	OrderInfoStruct *pOrderinfo;
	//断网了，暂时不上报
	if(!TCP_MessageFlag.SignIn)
	{
		return;
	}
	//没有完成的订单就不需要判断
	if(!EquipmentManage.OrderDeal.FinishOrderCnt)
	{
		return ;
	}
	//设置订单状态为打包完成
	for(i=0; i<EquipmentManage.OrderDeal.FinishOrderCnt; i++)
	{
		//对于PLC来说只有一种，制作完成就是打包完成，因为有可能机械手不工作
		pOrderinfo = &EquipmentManage.OrderDeal.FinishOrderInfo[i];
		//这里如果完成数组中的订单状态不是打包完成，就强制转换为打包完成
		if(pOrderinfo->OrderState != PackageFinish)
		{
			//第一种方式，判断机械臂原点
			
			pOrderinfo->OrderState = PackageFinish;
		
			if(pOrderinfo->OrderState == PackageFinish)
			{
				dzlog_debug("order %s package finish\r\n",pOrderinfo->OrderSn);
			}
		}
		else//订单完成每间隔10s上传一次112_也就是说订单状态会停留10s在打包完成
		{
			pOrderinfo->TimeOutCnt++;
			if(pOrderinfo->TimeOutCnt>10000)
			{
				pOrderinfo->TimeOutCnt = 0;
				CMD112_Handle(pOrderinfo);
			}
		}
	
	}	
	return ;
}
 

  /**
 ****************************************************************************************
 @brief:	ErrOrderHandle 完成订单处理处理
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-10-16
 ****************************************************************************************
  **/
void ErrOrderHandle()
{
	
	uint8_t i;
	OrderInfoStruct *pOrderinfo;
	//断网了，暂时不上报
	if(!TCP_MessageFlag.SignIn)
	{
		return;
	}
	//没有完成的订单就不需要判断
	if(!EquipmentManage.OrderDeal.ErrOrderCnt)
	{
		return ;
	}
	
	for(i=0; i<EquipmentManage.OrderDeal.ErrOrderCnt; i++)
	{
		pOrderinfo = &EquipmentManage.OrderDeal.ErrOrderInfo[i];
		pOrderinfo->TimeOutCnt++;
		//每15s钟发送一次108
		if(pOrderinfo->TimeOutCnt > 15000)
		{
			pOrderinfo->TimeOutCnt = 0;
			CMD108_Handle(pOrderinfo->OrderSn, pOrderinfo->OrderState);
		}
	
	}	
	return ;
}
  /**
 ****************************************************************************************
 @brief:	CookerManage 炒菜管理
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	龙川2019-07-08
 ****************************************************************************************
  **/
void *CookerManage(void  * argument)
{
	printf("%s\r\n",__func__);
	while(1)
	{
		//目前十代机器，主控先实现库存监控，其他的订单处理由后台和机器完成
		// CheckOrderFinish();
		// OrderManager();
		// ValidCookerCheck();//现在不做有效锅检测，防止下发103导致重复扣库存——库存监控时不用后台下单
		// FinishOrderHandle();
		// ErrOrderHandle();
		InventoryManager();
		usleep(1000);
	}
}
