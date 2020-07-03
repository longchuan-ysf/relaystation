/*************************************************************************************************** 
                                   xxxx公司
  
                  

文件:   CookerManager.h 
作者:   龙川
说明:   炒菜机管理头文件
***************************************************************************************************/
#ifndef __EQUIOMENTMANAGE_H
#define __EQUIOMENTMANAGE_H

#include "com.h"
#define TotalCookerNum 4//定义总共管理多少台炒菜机

#define QueueTotalLen (TotalCookerNum*2)//定义订单队列深度

//#define TotalRefrigertorNum 1//定义总共管理多少台冷柜

#define  CookTaskCode  1
#define  RefrigertorTaskCode  2

#define RUN_MODE_NORMAL 			0x00//正常模式
//只要bit0为1就是调试模式
#define RUN_MODE_DEBUG  			0x01//单独调试   验证与后台订单交互功能
#define RUN_MODE_DEBUG_Cook  		0x11//与炒菜机联调  验证与炒菜机订单交互制作功能
#define RUN_MODE_DEBUG_Refrigertor  0x21//与冷柜联调 验证与冷柜订单交互准备功能
#define RUN_MODE_IAP    			0x02//IAP模式

//#定义挡盒子电机状态
#define StopDishDCState_Open  1
#define StopDishDCState_Close 0

#define OrderInfo_Valid  1//订单在Flash中存储有效
#define OrderInfo_Invalid  0 //订单在Flash中存储无效

#define INIT_EquipmentNumber "YSFE092004010003"
//定义炒菜任务参数结构体
typedef struct
{
	uint8_t Step;//步骤
	uint8_t OperationCode;//操作码
	uint8_t Perform;//是否执行
	uint8_t OperationTime[2];//操作时长
	uint8_t RotateSpeed[2];//翻炒转速
	uint8_t Temperature;//电磁炉温度
	uint8_t Gear;//电磁炉档位
	uint8_t RSV[4];
}TaskParaStruct;
//定义订单状态
typedef enum
{
	Wait = 0u,//等待
	Prepare,//准备
	PrepareFinish,
	Cooking,//制作中
	CookFinish,//制作完成
	Packaging,//打包中
	PackageFinish,//打包完成

	Abnormal=0xFF,//订单异常

}OrderStateEnum;
//任务状态枚举
typedef enum
{
	Task_None = 0u,//没有任务
	Task_Receive,//任务接收成功
	Task_Wait,//任务等待中——可能在备食材
	Task_Ongoing,//任务进行中
	Task_Complete,//任务完成
	Task_Stop,//任务已停止
	Task_Error,//任务出错
} TaskStateEnum;
typedef enum
{
	Standby = 0U,//待机啥事都没做
	Ready,//就绪，接收到命令从待机状态切换
	Running,//运行
	Finish,//结束
	Warnning,//告警
} EquipmentStateEnum;
typedef struct
{
	uint32_t TimeCntTotal;//总的计数时间，分配预备任务给锅之后相应的增加
	uint32_t TimeTarget;//完成这道菜所用时间
	uint32_t CookerTime;//炒菜时间
	uint8_t TimeState;//0计数器无效1计数器启动
}TaskTimeCntStruct;
//定义订单处理信息
typedef struct
{
	uint8_t OrderSn[32];//订单编号
	uint8_t UserName[32];//用户名称
	uint8_t FoodMaterial[16];//菜品编号
	uint8_t TakeFoodCode[4];//取餐码
	uint8_t TimeStamp[4];//时间戳 
	uint8_t MenuSn[16];//菜谱编号
	uint8_t TotalStep;//菜谱总步数
	TaskParaStruct CookBook[32];//菜谱
	
	//后面这些事订单管理需要用到的
	
	OrderStateEnum OrderState;//订单状态
	OrderStateEnum RecordOrderState;//订单上一次状态
	uint8_t PlanCooker;//预计那口锅炒菜
	uint8_t Cooker;//实际那口锅炒菜
	uint8_t TaskSn[16];	//该订单对应的任务流水号_由于PLC是一个16位寄存器，所以只使用前面两位
	
	uint8_t row;//行号
	uint8_t column;//列号
	
	//订单命令流程的处理
	uint8_t ExecuteUnit;//当前订单的执行单元0代表没有使用，1冷柜，2炒菜机——后期还可能有饭柜
	
	uint32_t CMDHandile;//低16位代表运行接下来发送什么命令，高16位标识允许向哪个发送冷柜还是机器
	
	uint32_t TimeOutCnt;//订单超时
	
	uint8_t SaveValid;//Flash记录有效标志
	
	
	
	//订单打包用到的_6代机用
	uint16_t OrderPosition;//订单的位置
	//
}OrderInfoStruct;


//定义订单处理信息
//当UsedLen不为0的时候开始轮训订单状态及状态上报，每完成一个订单UsedLen--，FreeLen++
typedef struct
{
	uint8_t TotalLen;//队列总深度
	uint8_t ValidLen;//队列有效深度
	uint8_t UsedLen;//队列已用深度
	uint8_t FreeLen;//队列空闲深度
	//制作完成之前的订单存储
	OrderInfoStruct OrderInfo[QueueTotalLen];//订单详细信息
	
	//异常订单存储
#define Max_ErrOrder   	10//在打包流水线上最多处理多少个订单
	uint8_t ErrOrderCnt;//错误订单计数
	OrderInfoStruct ErrOrderInfo[Max_ErrOrder];//错误订单存储
	//制作完成订单存储
#define Max_FinishOrder   	10//在打包流水线上最多处理多少个订单
	uint8_t FinishOrderCnt;//完成订单计数
	OrderInfoStruct FinishOrderInfo[Max_FinishOrder];//完成订单存储
}OrderDealStruct;








//炒菜机管理——针对PLC控制的8代机做了些裁剪
typedef struct
{
	uint8_t ConnectState;//设备连接状态
			
	EquipmentStateEnum  EquipmentState;//炒菜机运行状态
	
	uint8_t EquipmentSn[16];//炒菜机编号

	uint32_t HighestAlarm;//炒菜机最高优先级告警记录
	uint32_t AlarmRecord[8];//告警记录
	uint32_t DisConnectTimeCnt;//重连计数
	uint8_t HeartBeatCnt;//心跳超时次数
	uint8_t HeartBeatFre;//心跳周期
	uint8_t BackgroundOnOffline;//后台上下线命令

}ControlManageStruct;//控制板结构体


typedef enum
{
	Enum_AddOil = 0u,//加油
	Enum_AddFood1,//加食材1
	Enum_AddFood2,//加食材2
	Enum_AddFood3,//加食材3
	Enum_AddFood4,//加食材4Box_Idle=0u,//空闲
	Enum_StirFry,//翻炒
	Enum_OutofPots,//出锅
	Enum_AddWater,//
	Enum_ChangeTemp,//变温
	Enum_ChangSpeed,//变速
	Enum_AddSauce1,//添加1号酱料
	Enum_AddSauce2,//添加2号酱料
	Enum_AddSauce3,//添加3号酱料
	Enum_AddSauce4,//添加4号酱料
	Enum_Wash=100,//洗锅
	Enum_RecieveBox,//接收盒子
	Enum_RecycleBox,//回收盒子	
} CookOperationCode;


struct tcp_client_dev
{
	bool tcp_connect_state;
	unsigned char serverip[16];
	unsigned char serverport[8];
	int sockfd;
};
typedef struct
{
	//上面这些是用在主控板管理
	uint32_t RunMode;//运行模式
	uint8_t EquipmentSn[16];//设备编号

	
	
	// struct tcp_pcb *tcp_client_pcb;//the TCP protocol control block 
	// struct tcp_pcb *modbus_client_pcb;//the TCP protocol control block 
	// uint8_t ConnectState;//0代表断开连接，1代表连接上,2代表连接上并且有报文通讯
	// uint8_t ModbusConnectState;//0代表断开连接，1代表连接

	bool enet_connect_state;
	bool RefrigertorRefresh;//冷柜同步后台数据
	struct tcp_client_dev tcp_background;
	struct tcp_client_dev tcp_modbus;
	uint32_t HeartbeatFre;//心跳包周期
	uint32_t HeartbeatTimeout;//心跳包超时次数
	uint8_t	HeartbeatCnt;//心跳计数
	
	uint8_t NetStackOverflow;
	uint32_t Timestamp;//时间戳
	
	
	EquipmentStateEnum  EquipmentState;//主控运行状态
	
	//主控的 订单管理结构体
	OrderDealStruct OrderDeal;//订单处理
	
//	//主控的炒菜机管理结构体
	ControlManageStruct CookerManage[TotalCookerNum];
//	
//	//主控冷柜管理结构体
	ControlManageStruct RefrigertorManage;
//	
//	MechanicalArmStateEnum MechanicalArmState[2];
	
	 
}EquipmentManangeStruct;


extern EquipmentManangeStruct EquipmentManage;


extern void OrderManager(void);
extern void *CookerManage(void * argument);
extern void EquipmentManagerInit(void);

#endif

