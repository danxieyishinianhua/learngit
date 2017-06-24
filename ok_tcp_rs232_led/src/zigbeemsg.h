#ifndef _ZIGBEEMSG_H_
#define _ZIGBEEMSG_H_
#include <unistd.h>

#include "INLcpMsg.h"
#pragma pack(1)
//通用的帧格式详见 RexBee无线通信协议-v2
typedef struct  {
	BYTE header;
	BYTE packetlen;
	WORD framecontrol;
	BYTE reserve1[6];
	BYTE srtaddr[4];
	BYTE reserve2[4];
	BYTE dstaddr[4];
	BYTE reserve3[6];
	BYTE clusterid[2];
	BYTE reserve4[2];
}EMBERCMDPACKET ;

#define WRITEFLAG 0X25
#define READFLAG 0X20//Type=0x38, COOID=0x20, Read
#define ALLPARAM 0X00

#define NODESTATUSOD 0X00
#define NETWORKPARAMOD 1001   //(0x03E9)
typedef struct
{
	USIGN32 ulPAN_ID;//当前无线网络的网络标识（PAN ID） 。此参数通过 OD 修改之后，节点复位后生效；如果使用 AT指令修改，则立刻生效。
	USIGN8 ucCurrent_Channel;//当 前 正 在 使 用 的 通 道 ， 取 值 范 围 11-26（0x0B-0x1A） 。对此参数的写操作将改变无线模块的通信频率。此参数通过
	SIGN8 cTransmit_Power;//射频发送功率。使用写操作修改这一参数，就可以调节其发送功率。取值范围是：-26dBm – +7dBm
	USIGN8 ucMax_Hops;//路由传递无线数据时，最大的传播半径。
	USIGN8 ucNetwork_Options;
	/*网络通信的选项，定义如下：
	Bit0：设置是否丢弃从无线端接收到的过短的透传数据
	=0：不丢弃过短的透传数据
	=1：如果透传数据长度过短（<3） ，则认为透传数据是由于干
	扰引起的，节点将丢弃接这些过短的透传数据
	Bit1-7：保留*/
	USIGN32 ulGroup_ID;//节点所属组别，当发送的数据使用组别来寻址时使用。
	USIGN32 ulChosen_COO;//本节点已经选择的 COO 地址。值为 0 则说明还没有接收到 COO 发送的数据。这是一个只读参数，写操作无效。
	USIGN16 uiProfile_ID;//行规标识
	USIGN16 uiPowerMode;//
	/*模块的射频模式。
	Bit0：是否使用 Boost 模式
	=0：Normal Mode
	=1：Boost Mode
	Bit1：是否使用外部功放（PA）
	=0：不使用外部 PA
	=1：使用外部 PA
	Bit1-7：保留*/
	USIGN8 ucNode_Age_Step;//设置节点 Age 增加的时间间隔，单位是秒。此参数不建议用户配置。
	USIGN8 ucPoll_MSG_Life;//需要发送给 ZED 的无线数据，是由 ZED 的 Parent 先进行缓存，然后等待 ZED 唤醒之后来查询（Poll）取走的。本参数设置 Parent
							//为 ZED 缓存数据的时间长度，单位是秒。某条无线数据缓存的时间超过本参数设置的时间长度之后，将被放弃以释放 Parent的存储空间。
	USIGN8 ucAnt_Sel;//外置功放的天线选择。=0：选择 PCB 天线 =1：选择 U.FL 天线
	USIGN8 aucReserved[9];
} Network_Parameter;

#define DATETIMEOD 1002 //(0x03EA)
typedef struct
{
	USIGN8 ucYear;
	USIGN8 ucMonth;
	USIGN8 ucDate;
	USIGN8 ucHour;
	USIGN8 ucMinute;
	USIGN8 ucSecond;
	USIGN8 ucWeek;
	USIGN8 ucStatus;
} Date_Time;

#define EXECCONTROLOD 1003//(0x03EB)
#define COMPORTOD 4000//（0x0FA0） 透明传输功能
#define TRIGGERPARAMOD 4700//（0x125C）
typedef struct
{
	USIGN8 aucWake_Trigger[2];
	USIGN8 ucReLoad_Token;//参数用于恢复所有参数的默认值。向此参数写入 1，则模块中的所有参数都将被恢复为默认值（即出厂设置值） ；写入其他值则不触发任何操作。
	USIGN8 ucReset;//参数用于使模块复位，向此参数写入一个非0 值 n，则模块将在n 秒后自动复位。
	USIGN8 ucBoot;//参数用于使模块进入 Bootloader 模式。向此参数写入一个非 0值 n，则模块将在 n 秒后自动进入 Bootloader 模式。
	USIGN8 ucJoin;//参数用于触发父/子节点之间的加入/离开操作
	USIGN8 ucMTO;//仅对 COO 节点有效，用于触发 COO 节点重新建立网络写入 1：触发重新建立网络，写入 0：无变化
} TRIGGER_PARAMETER;


typedef struct
{
	USIGN8 uiNode_Addr[4];
	SIGN8  cRSSI;
	USIGN8 ucFailure_Or_Age;
}NODE_DESC;


#define CHILDTABLE 1600//（0x0FA0） 子表
#define NEIGHBORTABLE 1500 //邻居表
//send Zigbee does off_line ,Between bt.c and tcp.c.void zigbee_off_line(struct zb_tree zb_data,uint8 check_code);#endif //INLCP_MSG_H_



#define NET_0  0//686E518C 0B
#define NET_1  1//686E518D 0B
#define NET_2  2//686E518E 0B
#define NET_3  3//686E518F 0B

#define NET_4  4//686E518C 0F
#define NET_5  5//686E518D 0F
#define NET_6  6//686E518E 0F
#define NET_7  7//686E518F 0F

#define NET_8  8//686E518C 14
#define NET_9  9//686E518D 14
#define NET_10 10//686E518E 14
#define NET_11 11//686E518F 14

#define NET_12 12//686E518C 19
#define NET_13 13//686E518D 19
#define NET_14 14//686E518E 19
#define NET_15 15//686E518F 19

struct zpconfig
{
	unsigned char panid[10]; //
	unsigned char channel;//
};

//广播包
struct ZLL_CONFIG
{
	unsigned char msg_head; //0x7e
	unsigned char msg_len;//6
	unsigned char msg_id;//消息ID
	unsigned char net_id;//网络ID
	unsigned char on_off;//1：开灯 0：开灯    交替发送，间隔2秒 
	unsigned char msg_end;//0x5e
};

struct Lamp_Info
{
	uint8 lamp_id[4];
	uint8 valu_1;//0-100 调试色温   101-201 调试RGB 
	uint8 valu_2;
	uint8 valu_3;
	uint8 valu_4;
};

#define MAXLAMPSCOUNT  16
#define HEADFLAG 0X7E  //消息头
#define ENDFLAG 0X5E   //消息尾
struct Lamps_Info
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char lamp_num;
	struct Lamp_Info info[MAXLAMPSCOUNT];
	unsigned char msg_end;
};

struct Get_Lamp_Status
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char lamp_id[4];
	unsigned char msg_end;
};

struct Get_Lamp_Status_Answer
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char lamp_id[4];
	unsigned char valu_1;		//0-100：色温亮度  101-201：RGB亮度
	unsigned char valu_2;		//				R
	unsigned char valu_3;		//				G
	unsigned char valu_4;		//				B
	unsigned short H_angle;		//水平角度值 0-2000
	unsigned short V_angle;		//垂直角度值
	unsigned short F_angle;		//焦距角度值
	unsigned char  V_move_flag; //上下移动状态，1-最下位置 2-中 3-上
	unsigned char msg_end;
};

struct rs232_send_basic
{
    unsigned char msg_head;
    unsigned char msg_len;
    unsigned char msg_id;
    unsigned char lamp_id[4];
    unsigned char msg_end;
};

enum
{
	ID_NET_Config  				=1,
	ID_NET_Reset   				=2,	
	ID_NET_HeartBeat   			=3,
	ID_NET_HeartBeat_Answer		=4,
	
	ID_DATA_Pad      			=10,
	ID_DATA_Pad_set_Rqt			=11,
	ID_DATA_Set_Scene_Rsp		=12,
	ID_DATA_ReSet_Scene		    =13,
    ID_DATA_Control_scene		=14,
    ID_DATA_Set_Control			=15,
	
	MSG_SET_LAMPS				=20,
	MSG_SET_LIGHTNESS			=21,
	MSG_SET_COLORTEMP			=22,
	MSG_SET_RGB				    =23,
	MSG_LIGHT_STATUS			=24,
	
	MSG_SET_MOTOR				=25,
	
	MSG_GET_STATUS				=30,
	MSG_GET_STATUS_Answer		=31,

	MSG_SET_Answer				=35,

	MSG_update_start			=40,
	MSG_update_ask				=41,
	MSG_update_answer			=42,
	MSG_update_finish			=43,

	MSG_Lamp_Bind_Ctrl		    =50,
	MSG_Lamp_Bind_Ctrl_Rsp	    =51,
	MSG_Lamp_Delete_Ctrl		=52,
	MSG_Lamp_Delete_Ctrl_Rsp	=53,

	MSG_Scene_Sync				=60,
	MSG_Scene_Sync_Rsp		    =61,
};

enum {
	LCP_LINK_INIT  			=0,
	LCP_LINK_INIT_RSP	    =1,
	
	LCP_LINK_KEEPALIVE		=2,
	LCP_LINK_KEEPALIVE_RSP	=3,

	LCP_LAMP_LOGIN			=4,
	LCP_LAMP_LOGIN_RSP		=5,

	LCP_LAMP_LOGOUT			=6,
	LCP_LAMP_LOGOUT_RSP		=7,

	LCP_SET_BRIGHTNESS_COLOR_TEMP			=8,
	LCP_SET_BRIGHTNESS_COLOR_TEMP_RSP		=9,
	LCP_SET_RGB								=10,
	LCP_SET_RGB_RSP							=11,
	
	LCP_GET_LAMP_INFO						=12,
	LCP_GET_LAMP_INFO_RSP					=13,

	LCP_SET_GROUP_LAMP_INFO					=14,
	LCP_SET_GROUP_LAMP_INFO_RSP				=15,

	LCP_SET_LAMP_INFO_ONE_BY_ONE			=16,
	LCP_SET_LAMP_INFO_ONE_BY_ONE_RSP		=17,
	
	LCP_SET_MOTOR_PARAMS					=18,
	LCP_SET_MOTOR_PARAMS_RSP				=19,

	LCP_CTRL_PANEL_LOGIN					=20,
	LCP_CTRL_PANEL_LOGIN_RSP				=21,
	
	LCP_CTRL_PANEL_LOGOUT					=22,
	LCP_CTRL_PANEL_LOGOUT_RSP				=23,

	LCP_SYNC_DEFAULT_SCENE_INFO				=24,
	LCP_SYNC_DEFAULT_SCENE_INFO_RSP			=25,

	LCP_SYNC_LAMP_LIST_BY_CTRL_PANEL		=26,
	LCP_SYNC_LAMP_LIST_BY_CTRL_PANEL_RSP	=27,
};


#pragma pack()



int OnButtonZgTransfer(char *srcmac, char *dstmac ,char *buffer, int buflen);
int packetZigbeedata(unsigned short cmd,char *dstbuf,char *srcbuf, int len, char *dstmac, char *srcmac);
void ReadOMCConfig(void);

#endif //ZIGBEE_MSG_H



