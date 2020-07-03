/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   Record.c 
作者:   龙川
说明:   炒菜机存储数据记录控制——保存在当前应用目录下record目录下的record_*文件下
	   设备参数文件record_syspara.xlsx
	   错误信息文件record_err.xlsx
	   订单信息文件record_order.xlsx
	   冷柜库存文件record_FoodMaterial.xlsx
***************************************************************************************************/
#include "Record.h"
#include "com.h"
SaveParaStruct SavePara;
FoodMaterial ReFoodMaterial[MAX_REFRIGERTOR_ROW][MAX_REFRIGERTOR_COLUMN];//每格存储菜品编号为16字节
static void debugf_savepara(void)
{
	#if 0
	debugf("EquipmentSN:%s\r\n",SavePara.EquipmentSN.parament);
	debugf("serverip:%s\r\n",SavePara.serverip.parament);
	debugf("serverport:%s\r\n",SavePara.serverport.parament);
	debugf("modbusip:%s\r\n",SavePara.modbusip.parament);
	debugf("modbusport:%s\r\n",SavePara.modbusport.parament);
	#endif
}
/**
 ****************************************************************************************
 @brief:	check_syspara 
 @Input：	NULL
 @Output：  NULL
 @Return	哪个参数检查没通过，相应的位就置1
 @Warning:	NULL
 @note: 	参数检查，看读到的参数是不是正确的
 ****************************************************************************************
  **/
static int check_syspara(void)
{
	int ret,check;
	struct in_addr addr;
	ret = 0;
	dzlog_debug("check EquipmentSN:%s",SavePara.EquipmentSN.parament);
	check = strlen((char *)SavePara.EquipmentSN.parament);
	if(!check)
	{
		ret |= (0x01<<0);
		dzlog_debug("   EquipmentSN err!\r\n");
	}
	else
	{
		dzlog_debug("   EquipmentSN OK!\r\n");
	}
	
	

	dzlog_debug("check serverip:%s",SavePara.serverip.parament);
	check = inet_pton(AF_INET, (char *)SavePara.serverip.parament, &addr);
	if(check!=1)
	{
		ret |= (0x01<<1);
		dzlog_debug(" serverip err!\r\n");
	}
	else
	{
		dzlog_debug(" serverip OK!\r\n");
	}
	
	
	dzlog_debug("check serverport:%s",SavePara.serverport.parament);
	check = atoi((char *)SavePara.serverport.parament);
	if(check<=0)
	{
		ret |= (0x01<<2);
		dzlog_debug("   serverport err!\r\n");
	}
	else
	{
		dzlog_debug("   serverport OK!\r\n");
	}
	
	

	dzlog_debug("check modbusip:%s",SavePara.modbusip.parament);
	check = inet_pton(AF_INET, (char *)SavePara.modbusip.parament, &addr);
	if(check != 1)
	{
		ret |= (0x01<<3);
		dzlog_debug("  modbusip err!\r\n");
	}
	else
	{
		dzlog_debug("  modbusip OK!\r\n");
	}
	
	
	dzlog_debug("check modbusport:%s",SavePara.modbusport.parament);
	check = atoi((char *)SavePara.modbusport.parament);
	if(check<=0)
	{
		ret |= (0x01<<4);
		dzlog_debug("  modbusport err!\r\n");
	}
	else
	{
		dzlog_debug("  modbusport OK!\r\n");
	}
	return ret;
}
 /**
 ****************************************************************************************
 @brief:	SysParaNameInit 
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	参数名称初始化——在读取参数文件时是使用名称匹配才知道哪个是哪个的参数
			所以一定要先初始化名称才能读取参数表
 ****************************************************************************************
  **/
void SysParaNameInit(void)
{
	memset(&SavePara,0,sizeof(SavePara));
	sprintf((char *)SavePara.EquipmentSN.name,"%s","EquipmentSN");
	sprintf((char *)SavePara.serverip.name,"%s","serverip");
	sprintf((char *)SavePara.serverport.name,"%s","serverport");
	sprintf((char *)SavePara.modbusip.name,"%s","modbusip");
	sprintf((char *)SavePara.modbusport.name,"%s","modbusport");
}
 /**
 ****************************************************************************************
 @brief:	SytemParaInit 
 @Input：	fp:参数保存的文件指针
			needreinit：每位代表该参数要重新初始化
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	初始化参数并保存——needreinit这个参数目前没有做
 ****************************************************************************************
  **/
void SytemParaInit(FILE *fp,int needreinit)
{
	int reinitflag;
	if(!fp)
		return;

	//所有数据从头开始写--如果需要重新初始化就赋新值写入
	//不需要重新初始化就使用原来的值写入
	fseek(fp,0,SEEK_SET);

	reinitflag = ((needreinit>>0)&0x0001);
	if(reinitflag)
	{
		dzlog_debug("reinit EquipmentSN\r\n");
		sprintf((char *)SavePara.EquipmentSN.parament,"%s","YSFE092004010005");
	}
	//将值写入文件中
	fprintf(fp,"%s\t%s\r\n",SavePara.EquipmentSN.name,SavePara.EquipmentSN.parament);
	
	reinitflag = ((needreinit>>1)&0x0001);
	if(reinitflag)
	{
		dzlog_debug("reinit serverip\r\n");
		sprintf((char *)SavePara.serverip.parament,"%s","47.107.141.44");
	}
	fprintf(fp,"%s\t%s\r\n",SavePara.serverip.name,SavePara.serverip.parament);

	reinitflag = ((needreinit>>2)&0x0001);
	if(reinitflag)
	{
		dzlog_debug("reinit serverport\r\n");
		sprintf((char *)SavePara.serverport.parament,"%s","8899");
	}
	fprintf(fp,"%s\t%s\r\n",SavePara.serverport.name,SavePara.serverport.parament);

	reinitflag = ((needreinit>>3)&0x0001);
	if(reinitflag)
	{
		dzlog_debug("reinit modbusip\r\n");
		sprintf((char *)SavePara.modbusip.parament,"%s","192.168.2.96");
	}
	fprintf(fp,"%s\t%s\r\n",SavePara.modbusip.name,SavePara.modbusip.parament);

	reinitflag = ((needreinit>>4)&0x0001);
	if(reinitflag)
	{
		dzlog_debug("reinit modbusport\r\n");
		sprintf((char *)SavePara.modbusport.parament,"%s","502");
	}
	fprintf(fp,"%s\t%s\r\n",SavePara.modbusport.name,SavePara.modbusport.parament);

	// sdebugf((char *)SavePara.modbusip,"%s","192.168.2.101");
	// sdebugf((char *)SavePara.modbusport,"%s","502");
	
	
}
  /**
 ****************************************************************************************
 @brief:	SysParaPowerOnInit 
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	系统参数上电初始化
 ****************************************************************************************
  **/
void SysParaPowerOnInit(void)
{	
	FILE *fp;
	int ret,i;
	int needreinit;//第几位代表第几个参数要重新初始化
	systempara tempsyspara;
	systempara *syspara;

	SysParaNameInit();
	fp = NULL;
	fp = fopen(RECORD_SYSPARA,"r+");
	fseek(fp, 0, SEEK_SET);
	if(!fp)
	{
		dzlog_error("open file %s err!\r\n",RECORD_SYSPARA);
		return;
	}
	needreinit = 0;
	//循环获取表格中的参数_参数表格式 参数名\t参数\r\n	
	while(!feof(fp))
	{
		ret = fscanf(fp,"%s\t%s\r\n",tempsyspara.name,tempsyspara.parament);
		if(ret < 0)
		{
			dzlog_error("read file %s err!\r\n",RECORD_SYSPARA);
		}
		else
		{
			dzlog_debug("name:%s,parament:%s\r\n",tempsyspara.name,tempsyspara.parament);
			//寻找匹配到的参数
			//获取保存参数的首个成员的地址
			syspara = (systempara *)&SavePara;
			for(i=0; i<sizeof(SaveParaStruct)/sizeof(systempara);i++)
			{
				if(!strcmp(tempsyspara.name,syspara->name))
				{
					dzlog_debug("find para name:%s\r\n",syspara->name);
					strcpy(syspara->parament,tempsyspara.parament);
					break;
				}
				//指向下一个成员
				syspara=syspara+1;
			}
		}
	}
	//判断读取的数据是否正确
	needreinit = check_syspara();
	if(needreinit)
	{
		dzlog_debug("check sys parament err! reinitialization sys parament!\r\n");
		dzlog_debug("needreinit = 0x%x\r\n",needreinit);
		SytemParaInit(fp,needreinit);
	}
	 fclose(fp);
	debugf_savepara();
}
  /**
 ****************************************************************************************
 @brief:	RefrigertorPowerOnInit 冷柜部分上电初始化
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	
 ****************************************************************************************
  **/
void RefrigertorPowerOnInit(void)
{		
	FILE *fp;
	fp = NULL;
	fp = fopen(RECORD_FOOD,"r+");
	if(!fp)
	{
		dzlog_error("open file %s err!\r\n",RECORD_FOOD);
		return;
	}
	/*文件的开头 */
   	fseek(fp, 0, SEEK_SET);
	fread(ReFoodMaterial,sizeof(ReFoodMaterial),1,fp);
	fclose(fp);
	fp=NULL;
	
}
  /**
 ****************************************************************************************
 @brief:	void RefrigertorWrite(void) 冷柜库存保存
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	
 ****************************************************************************************
  **/
void RefrigertorWrite(void)
{		
	FILE *fp;
	fp = NULL;
	fp = fopen(RECORD_FOOD,"w+");
	if(!fp)
	{
		dzlog_error("open file %s err!\r\n",RECORD_FOOD);
		return;
	}
	/*文件的开头 */
   	fseek(fp, 0, SEEK_SET);
	fwrite(ReFoodMaterial,sizeof(ReFoodMaterial),1,fp);
	fclose(fp);
	fp=NULL;
	
}

 /**
 ****************************************************************************************
 @brief:	SaveOrderPowerOnInit 
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	机器上电读取最前面10条订单数据，判断订单状态如果订单状态不在完成，就算异常
 ****************************************************************************************
  **/
void SaveOrderPowerOnInit(void)
{	
	
}
  /**
 ****************************************************************************************
 @brief:	SaveOrderRecordAdd 
 @Input：	pOrderInfoStruct 订单结构体指针
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	将最新的数据添加到数据表最前面
 ****************************************************************************************
  **/
void SaveOrderRecordAdd(OrderInfoStruct * pOrderInfoStruct)
{
	
	
}
  /**
 ****************************************************************************************
 @brief:	SaveOrderStateCahnge 
 @Input：	OrderSn 订单编号
			orderstate 
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	清除订单记录 清除指定订单号的记录
 ****************************************************************************************
  **/
void SaveOrderStateCahnge(uint8_t * OrderSn,OrderStateEnum  orderstate)
{
	
}
  /**
 ****************************************************************************************
 @brief:	SaveOrderRecordClean 
 @Input：	OrderSn 订单编号
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	清除订单记录 清除指定订单号的记录
 ****************************************************************************************
  **/
void SaveOrderRecordClean(uint8_t * OrderSn)
{
	
}
 /**
 ****************************************************************************************
 @brief:	SaveOrderRecordCleanAll 
 @Input：	NULL
 @Output：  NULL
 @Return	NULL
 @Warning:	NULL
 @note: 	清除所有订单记录
 ****************************************************************************************
  **/
void SaveOrderRecordCleanAll(void)
{
	return;
}









