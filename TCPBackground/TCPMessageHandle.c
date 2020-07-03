/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   MessageHandle.c 
作者:   龙川
说明:   后台报文处理
***************************************************************************************************/
#include "TCPBackground.h"

#include "TCPMessageHandle.h"
#include "fifo.h"
#include "fun.h"
#include "NetHandle.h"
#include "EquipmentManage.h"
#include "Record.h"
#define MESSAGE_TIMEOUT 5000
#define TCPMessage_TxBufferLen  512
#define TCPMessage_RxBufferLen  512+32//512是由于升级数据一次下发可能512字节，+32是增加头信息等

uint8_t TCPMessage_TXBuffer[TCPMessage_TxBufferLen];
uint8_t TCPMessage_RXBuffer[TCPMessage_RxBufferLen];

uint16_t TCPMessage_RxLen = 0;
uint16_t TCPMessage_TxLen = 0;

TCP_FlagStruct TCP_MessageFlag;

/**
****************************************************************************************
@brief:    TCPMessageAddData 将数据加到报文数组中
@Input：   data 数据指针
           dataformat 数据格式
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void TCPMessageAddData(uint8_t *data,DataFormat dataformat)
{
    uint8_t *p;
	if(!data)//输入参数检查
		return;
	
	if(dataformat == BYTE)
	{
		p = data;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p;
	}
	else if(dataformat == SHORT_LittleEndian)
	{
		p = data;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p++;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p;
	}
	else if(dataformat == SHORT_BigEndian)//大段模式从后面开始填数值
	{
		p = data+1;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p--;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p;
	}
	else if(dataformat == INT_LittleEndian)
	{
		p = data;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p++;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p++;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p++;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p;
	}
	else if(dataformat == INT_BigEndian)//大段模式从后面开始填数值
	{
		p = data+3;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p--;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p--;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p--;
		TCPMessage_TXBuffer[TCPMessage_TxLen++] = *p;
	}
}
/**
****************************************************************************************
@brief:    TCPMessageAddString 将一串数据加到报文发送数组中
@Input：   data 数据指针
           len 数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void TCPMessageAddString(uint8_t *data,uint16_t len)
{
	if(!data)
	{
		return;
	}
	if(len + TCPMessage_TxLen >= TCPMessage_TxBufferLen)
	{
		return;
	}
	memcpy(&TCPMessage_TXBuffer[TCPMessage_TxLen],data,len);
	TCPMessage_TxLen += len;
}
/**
****************************************************************************************
@brief:    TCPMessageTxInit 初始化发送报文
@Input：   CMD 命令代码
@Output：  NULL
@Return：  NULL
@Warning:  由于发送没有使用FIFO所以发送禁止多线程调用，否则会导致发送Buffer莫名被修改，或者加互斥锁   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void TCPMessageTxInit(uint16_t CMD)
{
	static uint8_t TCPMessageSN = 0;
	uint8_t data;
	TCPMessage_TxLen = 0;
	//给发送缓冲区上锁
	pthread_mutex_lock(&TCPBackground_mutex);

	memset(TCPMessage_TXBuffer,0,TCPMessage_TxBufferLen);
	//帧头
	data = BACKGROUND_SYNC_BYTE1;
	TCPMessageAddData(&data,BYTE);
	data = BACKGROUND_SYNC_BYTE2;
	TCPMessageAddData(&data,BYTE);
	//长度域
	TCPMessage_TxLen+=2;
	//序号域
	TCPMessageAddData(&TCPMessageSN,BYTE);
	TCPMessageSN++;
	//信息域
	data = 0;//不加密
	TCPMessageAddData(&data,BYTE);
	//命令代码
	TCPMessageAddData((uint8_t *)&CMD,SHORT_LittleEndian);
	//数据域和校验域后面添加
}
/**
****************************************************************************************
@brief:    MessageTxSnRewrite 
@Input：   sn 主控的序列号
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-4-18
****************************************************************************************
 **/
void TCPMessageTxSnRewrite(uint8_t sn)
{
	TCPMessage_TXBuffer[4] = sn ;
}
/**
****************************************************************************************
@brief:    TCPMessagePacking 将数据打包
@Input：   NULL
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void TCPMessagePacking(void)
{
	uint8_t Sum;
	TCPMessage_TxLen ++;//最后1字节校验位
	//添加长度域
	TCPMessage_TXBuffer[2] = TCPMessage_TxLen&0xff;
	TCPMessage_TXBuffer[3] = (TCPMessage_TxLen>>8)&0xff;
	
	//计算校验和
	Sum = CheckSum(TCPMessage_TXBuffer,TCPMessage_TxLen);	
	TCPMessage_TXBuffer[TCPMessage_TxLen-1] = Sum;
}
/**
****************************************************************************************
@brief:    TCPMessageSend 将数据发送
@Input：   NULL
@Output：  NULL
@Return：  NULL
@Warning:  NULL  
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void TCPMessageSend(void)
{
	TCPMessagePacking();


	if(TCP_Client_Send_Data(TCPMessage_TXBuffer,TCPMessage_TxLen))
	{

		dzlog_warn("Disconnect Send\n");
		pthread_mutex_unlock(&TCPBackground_mutex);
		EquipmentManage.tcp_background.tcp_connect_state = false;//发送出错重连		
		return;
	}
	pthread_mutex_unlock(&TCPBackground_mutex);
	dzlog_debug("TCP TX CMD = %04x:\n",TCPMessage_TXBuffer[6]|(TCPMessage_TXBuffer[7]<<8));
	printf_Hexbuffer(TCPMessage_TXBuffer,TCPMessage_TxLen);		

}
/**
****************************************************************************************
@brief:    TCPMessgeDecode 数据解码 然后将解码的数据放到Message_RXBuffer中
@Input：   NULL
@Output：  NULL
@Return：  函数执行情况 ERR_NONE  ERR_GENERAL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
 uint8_t TCPMessgeDecode(void)
 {
	 uint32_t len,TotalDataLen;
	 static uint8_t step = 0;
	 static uint32_t timeout;
	 uint8_t Sum;
	 
	 len = fifo_data_length(&TCPBackGroundFIFO.rxFifo);
	 if(len == 0)
		 return 0xff; 
	
	switch(step)
	{
		case 0://解析第一帧数据，获取总帧数
		{	
			if(len<4)
			{
				return 0xff;
			}
			fifo_out(&TCPBackGroundFIFO.rxFifo,TCPMessage_RXBuffer,4);//头
			if((TCPMessage_RXBuffer[0] == BACKGROUND_SYNC_BYTE1)&&(TCPMessage_RXBuffer[1] == BACKGROUND_SYNC_BYTE2))
			{
				TCPMessage_RxLen = 4;
				step++;
				timeout = 0;
			}
		}
		break;
		case 1://获取剩余的数据
		{
			len = fifo_data_length(&TCPBackGroundFIFO.rxFifo);
			TotalDataLen = TCPMessage_RXBuffer[2] + (TCPMessage_RXBuffer[3]<<8);
			
			if(len<TotalDataLen-4)//已经出队4个数据
			{
				timeout++;
				if(timeout>MESSAGE_TIMEOUT)
				{
					step = 0;
					TotalDataLen = 0;
					timeout = 0;
					return 0xff;
				}
				return 0x01;//等待所有数据传输完成
			}
			timeout = 0;
			fifo_out(&TCPBackGroundFIFO.rxFifo,&TCPMessage_RXBuffer[TCPMessage_RxLen],TotalDataLen-4);//出队剩余数据
			Sum = CheckSum(TCPMessage_RXBuffer,TotalDataLen-1);// 从起始域到数据域的累加和校验,不包括最后一字节的校验域
			if(TCPMessage_RXBuffer[TotalDataLen-1] == Sum)//校验通过
			{
				step = 0;
				dzlog_debug("TCP RX CMD = %04x:\n",TCPMessage_RXBuffer[6]|(TCPMessage_RXBuffer[7]<<8));
				printf_Hexbuffer(TCPMessage_RXBuffer,TotalDataLen);
				return 0;
				
			}
			else
			{
				step = 0;
				return 0xff;
			}
			
			
		}
		default:
			step = 0;
			break;

	}
	 return 0x01;
 }
void CMD03_Handle(uint8_t *data,uint8_t SN,uint16_t len)
{
	uint8_t state;
	EquipmentManage.Timestamp = data[0] + (data[1]<<8) + (data[2]<<16) + (data[3]<<24);

	TCPMessageTxInit(0x04);

	TCPMessageTxSnRewrite(SN);
	state = 0;
	TCPMessageAddData((uint8_t *)&state,BYTE);

	TCPMessageSend();//调用发送
}
 /**
****************************************************************************************
@brief:    CMD09_Handle 冷柜更新报文
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-13
****************************************************************************************
 **/
void CMD09_Handle(uint8_t *data,uint8_t SN,uint16_t len)
{
	uint8_t i,num,state;
	uint8_t Refrigertor[24];
	uint8_t *pData;
	RefrigertorFoodSnStruct *RefrigertorFoodSn;

	num = data[25];
	dzlog_debug("num = %d\r\n",num);
	pData = (data+26);
	
	state = 0;
	memcpy(Refrigertor,data,24);
	dzlog_debug("updata Refrigertor: %s\r\n",Refrigertor);
	for(i=0; i<num; i++)
	{
		RefrigertorFoodSn = (RefrigertorFoodSnStruct *)pData;
		if(RefrigertorFoodSn->row > MAX_REFRIGERTOR_ROW)
		{
			state = 1;
			break;
		}
		if(RefrigertorFoodSn->column > MAX_REFRIGERTOR_COLUMN)
		{
			state = 1;
			break;
		}
		
		//拷贝该通道的菜个数
		ReFoodMaterial[RefrigertorFoodSn->row - 1][RefrigertorFoodSn->column-1].num=RefrigertorFoodSn->num;  

		//拷贝该通道的菜品编号
		memcpy(ReFoodMaterial[RefrigertorFoodSn->row - 1][RefrigertorFoodSn->column-1].SN,RefrigertorFoodSn->FoodMaterial,16); 

		pData+= sizeof(RefrigertorFoodSnStruct);
		dzlog_debug("Update the inventory: row %d,column %d,num %d\r\n SN : %s\r\n",\
		RefrigertorFoodSn->row,RefrigertorFoodSn->column,RefrigertorFoodSn->num,RefrigertorFoodSn->FoodMaterial);
	}
	EquipmentManage.RefrigertorRefresh = true;
	TCPMessageTxInit(0x0A);

	TCPMessageTxSnRewrite(SN);

	TCPMessageAddData((uint8_t *)&state,BYTE);

	TCPMessageSend();//调用发送
}
 
 
 /**
****************************************************************************************
@brief:    CMD102_Handle 订单深度上报
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-13
****************************************************************************************
 **/
void CMD102_Handle(void)
{
	uint32_t Data;
	TCPMessageTxInit(0x102);
		
	Data = EquipmentManage.OrderDeal.TotalLen;//总长度
	TCPMessageAddData((uint8_t *)&Data,INT_LittleEndian);
	
	Data = EquipmentManage.OrderDeal.ValidLen;//有效长度
	TCPMessageAddData((uint8_t *)&Data,INT_LittleEndian);
	
	Data = EquipmentManage.OrderDeal.UsedLen;//使用长度
	TCPMessageAddData((uint8_t *)&Data,INT_LittleEndian);
	
	Data =  EquipmentManage.OrderDeal.FreeLen;//空闲长度
	TCPMessageAddData((uint8_t *)&Data,INT_LittleEndian);
	
	TCPMessageSend();//调用发送
}
   /**
  ****************************************************************************************
  @brief:	 CMD104_Handle 103响应
  @Input：   state 0更新成功 1更新失败
  @Output：  NULL
  @Return：  NULL
  @Warning:  NULL	
  @note:	 龙川 2019-7-10
  ****************************************************************************************
   **/
  void CMD104_Handle(uint8_t *OrderSn,uint8_t state,uint8_t SN)
  {
	  TCPMessageTxInit(0x104);
	  TCPMessageTxSnRewrite(SN);
	  TCPMessageAddString(OrderSn,32);
	  TCPMessageAddData((uint8_t *)&state,BYTE);
	  
	  TCPMessageSend();//调用发送
  }

   /**
****************************************************************************************
@brief:    CMD0D_Handle 单锅上下线命令
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-13
****************************************************************************************
 **/
void CMD0D_Handle(uint8_t *data,uint8_t SN,uint16_t len)
{
	uint8_t state;
	TCPMessageTxInit(0x0E);
	TCPMessageTxSnRewrite(SN);
	if(data[0]>=TotalCookerNum)
	{
		state = 1;
	}
	else
	{
		state = 0;
		EquipmentManage.CookerManage[data[0]].BackgroundOnOffline = data[1];
		EquipmentManage.CookerManage[data[0]].ConnectState = 0;
	}
	
	TCPMessageAddData((uint8_t *)&state,BYTE);
	
	TCPMessageAddData(data,BYTE);
	
	TCPMessageSend();//调用发送
}
  /**
 ****************************************************************************************
 @brief:	CMD103_Handle 更新任务队列
 @Input：	 data 数据
			Len  数据长度
 @Output：  NULL
 @Return：  NULL
 @Warning:	NULL   
 @note: 	龙川 2019-7-10
 ****************************************************************************************
  **/
 void CMD103_Handle(uint8_t *data,uint8_t SN,uint16_t len)
 {
 
	 uint8_t State,i,match;
	 uint8_t *pData;
	 uint32_t MenuSn;
	 uint16_t TaskSn,TaskSnTemp;
	// uint8_t RemainLen;//剩余长度
	 OrderInfoStruct *pOrderInfo;
	 
	// RemainLen = EquipmentManage.OrderDeal.ValidLen - EquipmentManage.OrderDeal.UsedLen;
	 dzlog_debug ("enter CMD103_Handle \n");
	 if(EquipmentManage.OrderDeal.FreeLen == 0)
	 {
		 State = 1;
		 dzlog_debug("RemainLen not enough ValidLen = %d,UsedLen = %d\r\n",EquipmentManage.OrderDeal.ValidLen,EquipmentManage.OrderDeal.UsedLen);
		 CMD104_Handle(data,State,SN);
		 return;
		 
	 }
	 //是否重复下单
	 if(EquipmentManage.OrderDeal.UsedLen)
	 {
		 for(i=0; i<EquipmentManage.OrderDeal.UsedLen; i++)
		 {
			 if(memcmp(&EquipmentManage.OrderDeal.OrderInfo[i].OrderSn,data,32) == 0)
			 {
				 State = 0;
				 dzlog_debug("equal %s,%s\r\n",EquipmentManage.OrderDeal.OrderInfo[i].OrderSn,data);
				 CMD104_Handle(data,State,SN);
				 return;
			 }
		 }
	 }
	 pOrderInfo = &EquipmentManage.OrderDeal.OrderInfo[EquipmentManage.OrderDeal.UsedLen];
	 memset(pOrderInfo,0,sizeof(OrderInfoStruct));
	 pData = data;
	 //复制订单编号
	 memcpy(pOrderInfo->OrderSn,pData,32);
	 pData+=32;
	 dzlog_debug("RX Order: %s,Position: %d\r\n",pOrderInfo->OrderSn,EquipmentManage.OrderDeal.UsedLen);
	 //复制用户编号
	 memcpy(pOrderInfo->UserName,pData,32);
	 pData+=32;
	 //复制菜品编号
	 memcpy(pOrderInfo->FoodMaterial,pData,16);
	 pData+=16;
	 //复制取餐码
	 memcpy(pOrderInfo->TakeFoodCode,pData,4);
	 pData+=4;
	 //复制取下单时间
	 memcpy(pOrderInfo->TimeStamp,pData,4);
	 pData+=4;
	 //后台计算的行号
	 pOrderInfo->row = *pData;
	 pData++;
	 //后台计算的列号
	 pOrderInfo->column = *pData;
	 pData++;
	 
	 MenuSn = str_to_int((char *)pData,16);
	 if((MenuSn == 0)||(MenuSn>0xffff))//非法编号
	 {
		State = 1;
		dzlog_debug("Err MenuSn %s\r\n",pData);
		CMD104_Handle(data,State,SN);
		return;
	 }
	 //复制菜谱编号
	 memcpy(pOrderInfo->MenuSn,pData,16);
	 pData+=16;
	 
	 //复制菜谱总步数
	 pOrderInfo->TotalStep = *pData;		 
	 pData++;
	 //复制菜谱参数
	 memcpy(pOrderInfo->CookBook,pData,pOrderInfo->TotalStep*sizeof(TaskParaStruct));

	//订单其他参数初始化
	 pOrderInfo->PlanCooker = 0xff;
	 pOrderInfo->Cooker = 0xff;
	 
	 //PLC不支持ASCII码，所以生成一个16位数保存在TaskSn[0]和TaskSn[1]中
	 do
	 {
		match = 0x0;
		 //随机一个16位的数
		TaskSn = rand()&0x3fff;
		//判断当前随机到的数是否与前面的重复
		for(i=0; i<EquipmentManage.OrderDeal.UsedLen; i++)
		 {
			 TaskSnTemp = EquipmentManage.OrderDeal.OrderInfo[i].TaskSn[0]+(EquipmentManage.OrderDeal.OrderInfo[i].TaskSn[1]<<8);
			 if(TaskSn == TaskSnTemp)
			 {
				match =  0xff;
				break;
			 }
		 }
	 }while(match == 0xff);
	 //保存随机到的数
	 pOrderInfo->TaskSn[0] = TaskSn&0xff;
	 pOrderInfo->TaskSn[1] = (TaskSn>>8)&0xff;
	 dzlog_debug("task sn = %d!!!!!!!!!!!!!\r\n",TaskSn);
//	 //以订单号的前16位为任务编码
//	 memcpy(pOrderInfo->TaskSn,pOrderInfo->OrderSn,16);
	 
	 pOrderInfo->ExecuteUnit = 0;
	 pOrderInfo->CMDHandile = 0;
	 pOrderInfo->TimeOutCnt = 0;

	 pOrderInfo->OrderState = Wait;
	 pOrderInfo->RecordOrderState = pOrderInfo->OrderState;
	 
	 pOrderInfo->SaveValid = OrderInfo_Valid;
	 EquipmentManage.OrderDeal.UsedLen++;
	 

	 State = 0;
	 CMD104_Handle(data,State,SN);

	 return;
 }


 void CMD124_Handle(uint8_t *OrderSn,uint8_t pan_num)
  {
	  TCPMessageTxInit(0x124);
	  TCPMessageTxSnRewrite(0xff);
	  TCPMessageAddString(OrderSn,32);
	  TCPMessageAddData((uint8_t *)&pan_num,BYTE);

	  dzlog_debug ("will send 0x0124 cmd， pan_num:%d\n", pan_num);
	  TCPMessageSend();//调用发送
  }

/**
****************************************************************************************
@brief:    CMD111_Handle 订单制作完成上报应答
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10——目前先不做重传——TCP一般都很稳的
****************************************************************************************
 **/
void CMD111_Handle(uint8_t *data,uint8_t SN,uint32_t Len)
{
	uint8_t i;
	OrderInfoStruct *pOrderinfo;
	//订单完成要移出数组
	for(i=0; i<EquipmentManage.OrderDeal.FinishOrderCnt; i++)
	{
		pOrderinfo = &EquipmentManage.OrderDeal.FinishOrderInfo[i];
		if(memcmp(pOrderinfo->OrderSn,data,32) == 0)
		{	
			//从Flash中删除
			//SaveOrderRecordClean(pOrderinfo->OrderSn);			
			//从完成数组中删除
			for(;i<EquipmentManage.OrderDeal.FinishOrderCnt-1; i++)
			{
				memcpy(&EquipmentManage.OrderDeal.FinishOrderInfo[i],\
							&EquipmentManage.OrderDeal.FinishOrderInfo[i+1],sizeof(OrderInfoStruct));
							
			}
			//将最后一个释放
			memset(&EquipmentManage.OrderDeal.FinishOrderInfo[EquipmentManage.OrderDeal.FinishOrderCnt-1],0,sizeof(OrderInfoStruct));
			EquipmentManage.OrderDeal.FinishOrderCnt = EquipmentManage.OrderDeal.FinishOrderCnt-1;
			return;
		}
	}
}
/**
****************************************************************************************
@brief:    CMD112_Handle 订单状态上报应答
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10——目前先不做重传
****************************************************************************************
 **/
void CMD112_Handle(OrderInfoStruct *pOrderinfo)
{
	TCPMessageTxInit(0x112);
			
	TCPMessageAddString(pOrderinfo->OrderSn,32);
	
	TCPMessageAddString(pOrderinfo->TimeStamp,4);
	
	TCPMessageAddString(pOrderinfo->TakeFoodCode,4);

	TCPMessageAddString(pOrderinfo->FoodMaterial,16);
	
	TCPMessageAddString(pOrderinfo->UserName,32);

	TCPMessageSend();//调用发送
}
/**
****************************************************************************************
@brief:    CMD116_Handle 炒菜机与订单对应关系上报
@Input：   pOrderinfo 订单信息结构体指针
           Cooker  改订单对应的炒菜机
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-10-21
****************************************************************************************
 **/
void CMD116_Handle(OrderInfoStruct *pOrderinfo,uint8_t Cooker)
{
	TCPMessageTxInit(0x116);
	TCPMessageAddString(pOrderinfo->OrderSn,32);
	TCPMessageAddData((uint8_t *)&Cooker,BYTE);
	TCPMessageSend();//调用发送
}
/**
****************************************************************************************
@brief:    CMD107_Handle 订单状态上报应答
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10——目前先不做重传
****************************************************************************************
 **/
void CMD107_Handle(uint8_t *data,uint8_t SN,uint32_t Len)
{
	uint8_t i;
	OrderInfoStruct *pOrderinfo;
	for(i=0; i<EquipmentManage.OrderDeal.ErrOrderCnt; i++)
	{
		pOrderinfo = &EquipmentManage.OrderDeal.ErrOrderInfo[i];
		if(memcmp(pOrderinfo->OrderSn,data,32) == 0)
		{
			//从Flash中删除
			//SaveOrderRecordClean(pOrderinfo->OrderSn);
			
			//从现有的错误订单数据中删除
			for(;i<EquipmentManage.OrderDeal.ErrOrderCnt-1; i++)
			{
				memcpy(&EquipmentManage.OrderDeal.ErrOrderInfo[i],\
							&EquipmentManage.OrderDeal.ErrOrderInfo[i+1],sizeof(OrderInfoStruct));
							
			}
			//将最后一个释放
			memset(&EquipmentManage.OrderDeal.ErrOrderInfo[EquipmentManage.OrderDeal.ErrOrderCnt-1],0,sizeof(OrderInfoStruct));
			EquipmentManage.OrderDeal.ErrOrderCnt = EquipmentManage.OrderDeal.ErrOrderCnt-1;
			return;
		}
	}
		
}
 /**
****************************************************************************************
@brief:    CMD108_Handle 订单状态上报
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD108_Handle(uint8_t *orderSn,uint8_t State)
{
	
	TCPMessageTxInit(0x108);
			
	TCPMessageAddString(orderSn,32);
	
	TCPMessageAddData((uint8_t *)&State,BYTE);
	
	TCPMessageSend();//调用发送
}

 /**
 ****************************************************************************************
 @brief:	CMD10B_Handle 请求订单详细参数响应
 @Input：	 data 数据
			Len  数据长度
 @Output：  NULL
 @Return：  NULL
 @Warning:	NULL   
 @note: 	龙川 2019-7-10
 ****************************************************************************************
  **/
 void CMD10B_Handle(uint8_t *data,uint32_t Len)
 {
	 
 }
 /**
 ****************************************************************************************
 @brief:	CMD10C_Handle 订单详细参数请求
 @Output：  NULL
 @Return：  NULL
 @Warning:	NULL   
 @note: 	龙川 2019-7-10
 ****************************************************************************************
  **/
 void CMD10C_Handle(uint8_t *orderSn)
 {
	 
	 
 }

 /**
****************************************************************************************
@brief:    CMD205_Handle 心跳应答
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD201_Handle(uint8_t *data,uint8_t SN,uint16_t len)
{
	//心跳计数清零
	EquipmentManage.HeartbeatCnt = 0;
	TCP_MessageFlag.HeartBeat = 0;
}
/**
****************************************************************************************
@brief:    CMD202_Handle 心跳包
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD202_Handle(void)
{
	uint8_t i;
	static uint32_t HeartBeatSn = 0;
	TCP_MessageFlag.HeartBeat = 1;
	EquipmentManage.HeartbeatCnt++;
	TCPMessageTxInit(0x202);
	
	TCPMessageAddData((uint8_t *)&HeartBeatSn,INT_LittleEndian);
	HeartBeatSn++;
	TCPMessageAddData((uint8_t *)&EquipmentManage.Timestamp,INT_LittleEndian);//时间戳
	
	//炒菜机连接状态
	for(i=0; i<TotalCookerNum; i++)
	{
		TCPMessageAddData(&i,BYTE);
		TCPMessageAddData(&EquipmentManage.CookerManage[i].ConnectState,BYTE);
	}
	
	//冷柜连接状态
	i=0x10;
	TCPMessageAddData(&i,BYTE);
	TCPMessageAddData(&EquipmentManage.RefrigertorManage.ConnectState,BYTE);
	
	TCPMessageSend();//调用发送
}

/**
****************************************************************************************
@brief:    CMD202_Handle 心跳包
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD204_Handle(void)
{
	uint8_t i,EquipmentState;

	TCPMessageTxInit(0x204);
	
	
	//炒菜机状态
	for(i=0; i<TotalCookerNum; i++)
	{
		TCPMessageAddData(&i,BYTE);
		EquipmentState = (uint8_t)EquipmentManage.CookerManage[i].EquipmentState;
		TCPMessageAddData(&EquipmentState,BYTE);
	}
	
	//冷柜状态
	i=0x10;
	TCPMessageAddData(&i,BYTE);
	EquipmentState = (uint8_t)EquipmentManage.RefrigertorManage.EquipmentState;
	TCPMessageAddData(&EquipmentState,BYTE);
	
	TCPMessageSend();//调用发送
}
 /**
****************************************************************************************
@brief:    CMD205_Handle 签到应答
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-13
****************************************************************************************
 **/
void CMD205_Handle(uint8_t *data,uint8_t SN,uint16_t len)
{
	
	if(data[0] == 0)
	{
		TCP_MessageFlag.SignIn = 1;//注册成功
		EquipmentManage.tcp_background.tcp_connect_state = true;
		EquipmentManage.HeartbeatCnt = 0;
		TCP_MessageFlag.HeartBeat = 0;
		//签到成功后上报队列深度
		CMD102_Handle();
	}
}

 /**
****************************************************************************************
@brief:     CMD206_Handle 签到
@Input：    NULL
@Output：   NULL
@Return：   NULL
@Warning:   NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD206_Handle(void)
{

	uint32_t Random;
	TCP_CMD206Struct CMD206;
	memset(&CMD206,0,sizeof(TCP_CMD206Struct));
	EquipmentManage.RefrigertorRefresh = false;
	memcpy(CMD206.EquipmentSn,EquipmentManage.EquipmentSn,16);
	CMD206.Ecrypt = 0;
	
	CMD206.HeartbeatFre[0] = EquipmentManage.HeartbeatFre&0xff;
	CMD206.HeartbeatFre[1] = (EquipmentManage.HeartbeatFre>>8)&0xff;
	CMD206.HeartbeatFre[2] = (EquipmentManage.HeartbeatFre>>16)&0xff;
	CMD206.HeartbeatFre[3] = (EquipmentManage.HeartbeatFre>>24)&0xff;
	
	CMD206.HeartbeatTimeout[0] = EquipmentManage.HeartbeatTimeout&0xff;
	CMD206.HeartbeatTimeout[1] = (EquipmentManage.HeartbeatTimeout>>8)&0xff;
	CMD206.HeartbeatTimeout[2] = (EquipmentManage.HeartbeatTimeout>>16)&0xff;
	CMD206.HeartbeatTimeout[3] = (EquipmentManage.HeartbeatTimeout>>24)&0xff;
	
	CMD206.RunMode = EquipmentManage.RunMode;
	
	Random = rand();//伪随机数
	
	CMD206.Random[0] = Random&0xff;
	CMD206.Random[1] = (Random>>8)&0xff;
	CMD206.Random[2] = (Random>>16)&0xff;
	CMD206.Random[3] = (Random>>24)&0xff;
	
	
	TCPMessageTxInit(0x206);
	
	TCPMessageAddString((uint8_t *)&CMD206,sizeof(CMD206));
	
	TCPMessageSend();//调用发送
}
/**
****************************************************************************************
@brief:    CMD208_Handle 告警
@Input：   data 数据
           Len  数据长度
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD208_Handle(void)
{
	uint8_t i;

	TCPMessageTxInit(0x208);
	
	
	//炒菜机告警
	for(i=0; i<TotalCookerNum; i++)
	{
		TCPMessageAddData(&i,BYTE);
	
		TCPMessageAddString((uint8_t *)EquipmentManage.CookerManage[i].AlarmRecord,32);
	}
	
	//冷柜告警
	i=0x10;
	TCPMessageAddData(&i,BYTE);
	TCPMessageAddString((uint8_t *)EquipmentManage.RefrigertorManage.AlarmRecord,32);
	
	TCPMessageSend();//调用发送
}
 /**
****************************************************************************************
@brief:     CMD20C_Handle 库存消耗告知
@Input：    NULL
@Output：   NULL
@Return：   NULL
@Warning:   NULL   
@note:     龙川 2019-7-10
****************************************************************************************
 **/
void CMD20C_Handle(uint8_t row ,uint8_t column,uint16_t FoodMaterial)
{

	uint8_t Data;
	uint8_t SN[24];
	uint32_t num;
	TCPMessageTxInit(0x20C);
	memset(SN,0,sizeof(SN));
	sprintf((char *)SN,"%d",FoodMaterial);
	TCPMessageAddString((uint8_t *)SN,16);
	//冷柜编号_目前就一个冷柜默认就和主控编号一致
	memset(SN,0,sizeof(SN));
	memcpy(SN,EquipmentManage.EquipmentSn,16);
	TCPMessageAddString((uint8_t *)SN,24);

	Data = 0;
	TCPMessageAddData(&Data,BYTE);

	TCPMessageAddData(&row,BYTE);

	TCPMessageAddData(&column,BYTE);
	//不是0xff说明找到了
	if(row!=0xff)
	{
		//变更前库存
		num = ReFoodMaterial[row-1][column-1].num;
		TCPMessageAddData((uint8_t *)&num,INT_LittleEndian);

		//变更后库存
		if(ReFoodMaterial[row-1][column-1].num)
		{
			ReFoodMaterial[row-1][column-1].num--;
		}
		num = ReFoodMaterial[row-1][column-1].num;
		TCPMessageAddData((uint8_t *)&num,INT_LittleEndian);
	}
	else
	{
		num = 0xffffffff;
		TCPMessageAddData((uint8_t *)&num,INT_LittleEndian);
		num = 0xffffffff;
		TCPMessageAddData((uint8_t *)&num,INT_LittleEndian);
	}
	

	TCPMessageSend();//调用发送
}

/**
****************************************************************************************
@brief:    MessageRxHandle 接收数据解析
@Input：   NULL
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-4-26
****************************************************************************************
 **/
void  TCPMessageRxHandle()
{
	TCP_Message *BackGroundMassage;
	uint16_t CMD,len;
	BackGroundMassage = (TCP_Message *)TCPMessage_RXBuffer;
	//获取命令代码
	CMD = BackGroundMassage->CMD[0]|(BackGroundMassage->CMD[1]<<8);
	len = BackGroundMassage->Length_L|(BackGroundMassage->Length_H<<8);

	//执行命令代码
	switch(CMD)
	{
		case 0x0001:
		{
			
		}
		break;
		case 0x003:
		{
			CMD03_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN, len - 9);
		}
		break;
		case 0x09:
		{
			CMD09_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN, len - 9);
		}
		break;
		case 0x0D:
		{
			CMD0D_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN, len - 9);
		}
		break;
		case 0x103:
		{
			CMD103_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN,len - 9);
		}
		break;
		case 0x107:
		{
			CMD107_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN,len - 9);
		}
		break;
		case 0x111:
		{
			CMD111_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN,len - 9);
		}
		break;
		
		case 0x201:
		{
			CMD201_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN,len - 9);
		}
		break;
		case 0x205:
		{
			CMD205_Handle((uint8_t *)&BackGroundMassage->data,BackGroundMassage->MessageSN,len - 9);
		}
		break;
		default:
			break;
	}

	
}
 /**
****************************************************************************************
@brief:    MessageHandle 报文交互处理
@Input：   NULL
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-4-26
****************************************************************************************
 **/
void StateChangeHandle(void)
{
	static uint8_t UsedLen = 0;
	uint8_t  OrderStateLen,i;
	OrderStateEnum OrderState;
	OrderStateEnum RecordOrderState;
	//队列长度发生变化
	if(UsedLen != EquipmentManage.OrderDeal.UsedLen)
	{
		UsedLen = EquipmentManage.OrderDeal.UsedLen;
		CMD102_Handle();
	}
	OrderStateLen = EquipmentManage.OrderDeal.UsedLen+EquipmentManage.OrderDeal.ErrOrderCnt\
					+EquipmentManage.OrderDeal.FinishOrderCnt;
	if(OrderStateLen)
	{
		for(i=0; i<EquipmentManage.OrderDeal.UsedLen; i++)
		{
			OrderState = EquipmentManage.OrderDeal.OrderInfo[i].OrderState;
			RecordOrderState = EquipmentManage.OrderDeal.OrderInfo[i].RecordOrderState;
			if(OrderState != RecordOrderState)
			{
				if(OrderState != Abnormal)//不是异常状态上传，异常状态让订单进入异常数组中处理
				{
					//只有制作中的订单才会计算超时
					EquipmentManage.OrderDeal.OrderInfo[i].TimeOutCnt = 0;
					CMD108_Handle(EquipmentManage.OrderDeal.OrderInfo[i].OrderSn, OrderState);
				}
			}
			EquipmentManage.OrderDeal.OrderInfo[i].RecordOrderState = OrderState;
		}
//		
//		for(i=0; i<EquipmentManage.OrderDeal.ErrOrderCnt; i++)
//		{
//			OrderState = EquipmentManage.OrderDeal.ErrOrderInfo[i].OrderState;
//			RecordOrderState = EquipmentManage.OrderDeal.ErrOrderInfo[i].RecordOrderState;
//			if(OrderState != RecordOrderState)
//			{
//				CMD108_Handle(EquipmentManage.OrderDeal.ErrOrderInfo[i].OrderSn, OrderState);
//			}
//			EquipmentManage.OrderDeal.ErrOrderInfo[i].RecordOrderState = OrderState;
//		}
//		
		for(i=0; i<EquipmentManage.OrderDeal.FinishOrderCnt; i++)
		{
			OrderState = EquipmentManage.OrderDeal.FinishOrderInfo[i].OrderState;
			RecordOrderState = EquipmentManage.OrderDeal.FinishOrderInfo[i].RecordOrderState;
			if(OrderState != RecordOrderState)
			{
				CMD108_Handle(EquipmentManage.OrderDeal.FinishOrderInfo[i].OrderSn, OrderState);
			}
			EquipmentManage.OrderDeal.FinishOrderInfo[i].RecordOrderState = OrderState;
		}
	}
}

 /**
****************************************************************************************
@brief:    MessageHandle 报文交互处理
@Input：   NULL
@Output：  NULL
@Return：  NULL
@Warning:  NULL   
@note:     龙川 2019-4-26
****************************************************************************************
 **/
void TCPMessageHandle(void)
{
//	uint8_t i;
	if(EquipmentManage.tcp_background.tcp_connect_state == false)
		return;
	//debugf ("TCPMessageHandle TCPBackground_1sec:%d\n", TCPBackground_1sec);
	 //检测是否收到报文
	if(TCPMessgeDecode() == 0)
	{
		//收到的报文处理
		TCPMessageRxHandle();
	}
	if(TCPBackground_1secFlag)
	{
		
		if(TCP_MessageFlag.SignIn)
		{
			if(TCPBackground_1sec%15 == 0)
			{
				CMD102_Handle();
			}
			if(TCPBackground_1sec%60 == 0)
			{
				CMD204_Handle();
			}
			if(TCPBackground_1sec%600 == 0)
			{
				CMD208_Handle();
			}
			// if(TCPBackground_1sec%600 == 0)
			// {
			// 	CMD206_Handle();//每隔10分钟重新签到，对一下库存
			// }
			if((TCPBackground_1sec%EquipmentManage.HeartbeatFre == 0))
			{
				CMD202_Handle();
				if(TCP_MessageFlag.HeartBeat)//心跳包发送，开始计时确定连接存活
				{
					if(EquipmentManage.HeartbeatCnt > EquipmentManage.HeartbeatTimeout)
					{
						EquipmentManage.tcp_background.tcp_connect_state = false;//心跳超时，重连
						EquipmentManage.HeartbeatCnt = 0;
						dzlog_warn("Disconnect Timeout\n");
					}
				}
			}
			//状态改变处理
			StateChangeHandle();
		}
		else
		{
			//debugf ("TCPBackground_1sec:%d\n", TCPBackground_1sec);
			if(TCPBackground_1sec%15 == 0)
			{
				
				CMD206_Handle();
			}
			
		}
	}
	
}
 
 
 
 
 
