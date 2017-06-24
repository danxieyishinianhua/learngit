#ifndef _NODEARRAY_H_
#define _NODEARRAY_H_
#include <sys/types.h>
#include <stdbool.h>
#include "INLcpMsg.h"
#include "zigbeemsg.h"
#include "fun_msgpack.h"

#define MAXZBNODECOUNT 100  //ֻ֧��100���ڵ�
#define MAXNEIGHBORCOUNT 12

#define MAXBUFLEN 20
#define CMDTRYCOUNT 4  //����Դ���
#define CMDTIMEOUT 1  //3//seconds
#define CMDINTERVAL  100 //���� ZIGBEE������
#define NEIGHBORINTERVAL  300//�ھӱ���ѯ���


extern int fd_A[1];


#pragma pack(1)//���ýṹ��ı߽����Ϊ1���ֽڣ�Ҳ���������������ڴ����������洢�ġ�
typedef struct
{
	USIGN8  cmdtrycount;
	USIGN8	cmdtimeout;
	uint32	cmdsendinterval;
	USIGN16	neighborinterval;
	char    panid[10];
	USIGN8  channel;	
	USIGN8  networkenable;
	char    zigbeecooaddr[32];
} ZBNETWORKPARAM;

typedef struct
{
	char serverip[32];
	USIGN16 serverport;
} ZBOMCPARAM;

typedef struct {
   uint8 macaddr[8];
   uint8 nwkaddr[2];
   int zbnodelive_val;//ZB_OFF_LINE �� ZB_ON_LINE	
   char testalivecount;
   long testtime;
   int neighborcount;
   int socket;
   char loginflag;
   long logintime;
   char heartbeatfailcount;
   char heartbeatinterval;
   long heartbeattime;
   long socketstarttime;//socket ������ʱ�� ���ʧ������T��������
   NODE_DESC neighbor[MAXNEIGHBORCOUNT];//12
   long commandtime;
   uint8 cmdtrycount;
   uint8 cmdbuflen;
   uint8 cmdbuf[MAXBUFLEN];//20
   uint8 lamp_type;//�Ƶ�����ָ����ʲô
   uint32 power;
   uint8 resend_cmd_flag;//�ط������־λ
   uint8 bringhtness;//����
   uint8 colortemp;//ɫ��
   uint8 lightness;//RGB����
   uint8 red;
   uint8 green;
   uint8 blue;
   uint8 scene_key;//����ָ���龰���ĸ�����
}zbnode, *pzbnode;

typedef struct
{
    int userflag[MAXZBNODECOUNT];//100
	zbnode node[MAXZBNODECOUNT];//100
}zbnodearray, *pzbnodearray;

#pragma pack()

extern ZBNETWORKPARAM g_zbnetworkparam;
extern ZBOMCPARAM g_zbomcparam;
extern pthread_mutex_t nodearraymutex;

int  addNewzbNode(pzbnode pnode );
int  delzbNode(int index);
pzbnode getNODEbyIndex(int index);
pzbnode getNODEbysocket(int socket);
void zbnodearrayInit();
void zbnodearrayDestory();
pzbnode gettestneighborNode(long currentime);
void printnode(pzbnode node);
void nodearray_print();
void updateTestneighborTime(char *mac,long time );
void updateneighbor(char *mac,char *nodebuf, int nodebuflen);
void updatenodesocket(char *mac, int socket);
bool isExistNode(char *mac);
void HandlezbnodeState();
void updatenodeNode(pzbnode node);
void CloseSocketbyindex(int index);
void updateHeartbeatTime(int index );
void updatezbnodelive_val(char *mac,int var );
void setzbnodecmdbuf(char *mac,char *cmdbuf, int buflen );
int getzbnodecmdbuf(int index, char *cmdbuf);
void Deletenodecmdbuf(char *dstmac, int cmd);
int getsocketbymac(char *mac);
void SaveZigbeeNode(void);
extern char IsZigbeeDebug();
void NodeTableHandle(char *databuf, int databuflen);
int getzigbeecmd(char *zbcmd,  char *omccmd);
int getzigbeeMAC(char *zbcmd);

int re_send_light_cmd(void);
void check_light_status_answer(char *str);
int read_lamp_info(struct LampInfo temp);
unsigned char add_zbnode_light_parm(char *mac,uint8 brightness,uint8 colortemp,uint8 r,uint8 g,uint8 b,uint8 flag);
void zigbee_node_heratbeat_outtime(long time);
void check_login_flag(uint8 flag);
void send_lamps_info_to_windows_client(void);
void send_server_info_to_windows_client(void);
void set_lamp_scene(char *str,uint8 key);
void check_lamp_scene(char *str,uint8 key);




#endif

