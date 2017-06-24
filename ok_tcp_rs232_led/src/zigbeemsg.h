#ifndef _ZIGBEEMSG_H_
#define _ZIGBEEMSG_H_
#include <unistd.h>

#include "INLcpMsg.h"
#pragma pack(1)
//ͨ�õ�֡��ʽ��� RexBee����ͨ��Э��-v2
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
	USIGN32 ulPAN_ID;//��ǰ��������������ʶ��PAN ID�� ���˲���ͨ�� OD �޸�֮�󣬽ڵ㸴λ����Ч�����ʹ�� ATָ���޸ģ���������Ч��
	USIGN8 ucCurrent_Channel;//�� ǰ �� �� ʹ �� �� ͨ �� �� ȡ ֵ �� Χ 11-26��0x0B-0x1A�� ���Դ˲�����д�������ı�����ģ���ͨ��Ƶ�ʡ��˲���ͨ��
	SIGN8 cTransmit_Power;//��Ƶ���͹��ʡ�ʹ��д�����޸���һ�������Ϳ��Ե����䷢�͹��ʡ�ȡֵ��Χ�ǣ�-26dBm �C +7dBm
	USIGN8 ucMax_Hops;//·�ɴ�����������ʱ�����Ĵ����뾶��
	USIGN8 ucNetwork_Options;
	/*����ͨ�ŵ�ѡ��������£�
	Bit0�������Ƿ��������߶˽��յ��Ĺ��̵�͸������
	=0�����������̵�͸������
	=1�����͸�����ݳ��ȹ��̣�<3�� ������Ϊ͸�����������ڸ�
	������ģ��ڵ㽫��������Щ���̵�͸������
	Bit1-7������*/
	USIGN32 ulGroup_ID;//�ڵ�������𣬵����͵�����ʹ�������Ѱַʱʹ�á�
	USIGN32 ulChosen_COO;//���ڵ��Ѿ�ѡ��� COO ��ַ��ֵΪ 0 ��˵����û�н��յ� COO ���͵����ݡ�����һ��ֻ��������д������Ч��
	USIGN16 uiProfile_ID;//�й��ʶ
	USIGN16 uiPowerMode;//
	/*ģ�����Ƶģʽ��
	Bit0���Ƿ�ʹ�� Boost ģʽ
	=0��Normal Mode
	=1��Boost Mode
	Bit1���Ƿ�ʹ���ⲿ���ţ�PA��
	=0����ʹ���ⲿ PA
	=1��ʹ���ⲿ PA
	Bit1-7������*/
	USIGN8 ucNode_Age_Step;//���ýڵ� Age ���ӵ�ʱ��������λ���롣�˲����������û����á�
	USIGN8 ucPoll_MSG_Life;//��Ҫ���͸� ZED ���������ݣ����� ZED �� Parent �Ƚ��л��棬Ȼ��ȴ� ZED ����֮������ѯ��Poll��ȡ�ߵġ����������� Parent
							//Ϊ ZED �������ݵ�ʱ�䳤�ȣ���λ���롣ĳ���������ݻ����ʱ�䳬�����������õ�ʱ�䳤��֮�󣬽����������ͷ� Parent�Ĵ洢�ռ䡣
	USIGN8 ucAnt_Sel;//���ù��ŵ�����ѡ��=0��ѡ�� PCB ���� =1��ѡ�� U.FL ����
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
#define COMPORTOD 4000//��0x0FA0�� ͸�����书��
#define TRIGGERPARAMOD 4700//��0x125C��
typedef struct
{
	USIGN8 aucWake_Trigger[2];
	USIGN8 ucReLoad_Token;//�������ڻָ����в�����Ĭ��ֵ����˲���д�� 1����ģ���е����в����������ָ�ΪĬ��ֵ������������ֵ�� ��д������ֵ�򲻴����κβ�����
	USIGN8 ucReset;//��������ʹģ�鸴λ����˲���д��һ����0 ֵ n����ģ�齫��n ����Զ���λ��
	USIGN8 ucBoot;//��������ʹģ����� Bootloader ģʽ����˲���д��һ���� 0ֵ n����ģ�齫�� n ����Զ����� Bootloader ģʽ��
	USIGN8 ucJoin;//�������ڴ�����/�ӽڵ�֮��ļ���/�뿪����
	USIGN8 ucMTO;//���� COO �ڵ���Ч�����ڴ��� COO �ڵ����½�������д�� 1���������½������磬д�� 0���ޱ仯
} TRIGGER_PARAMETER;


typedef struct
{
	USIGN8 uiNode_Addr[4];
	SIGN8  cRSSI;
	USIGN8 ucFailure_Or_Age;
}NODE_DESC;


#define CHILDTABLE 1600//��0x0FA0�� �ӱ�
#define NEIGHBORTABLE 1500 //�ھӱ�
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

//�㲥��
struct ZLL_CONFIG
{
	unsigned char msg_head; //0x7e
	unsigned char msg_len;//6
	unsigned char msg_id;//��ϢID
	unsigned char net_id;//����ID
	unsigned char on_off;//1������ 0������    ���淢�ͣ����2�� 
	unsigned char msg_end;//0x5e
};

struct Lamp_Info
{
	uint8 lamp_id[4];
	uint8 valu_1;//0-100 ����ɫ��   101-201 ����RGB 
	uint8 valu_2;
	uint8 valu_3;
	uint8 valu_4;
};

#define MAXLAMPSCOUNT  16
#define HEADFLAG 0X7E  //��Ϣͷ
#define ENDFLAG 0X5E   //��Ϣβ
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
	unsigned char valu_1;		//0-100��ɫ������  101-201��RGB����
	unsigned char valu_2;		//				R
	unsigned char valu_3;		//				G
	unsigned char valu_4;		//				B
	unsigned short H_angle;		//ˮƽ�Ƕ�ֵ 0-2000
	unsigned short V_angle;		//��ֱ�Ƕ�ֵ
	unsigned short F_angle;		//����Ƕ�ֵ
	unsigned char  V_move_flag; //�����ƶ�״̬��1-����λ�� 2-�� 3-��
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



