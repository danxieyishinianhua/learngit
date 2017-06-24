#ifndef _FUN_MSGPACK_H_
#define _FUN_MSGPACK_H_

#include <fcntl.h>
#include <msgpack.h>
#include "zigbeemsg.h"
#include <sys/types.h>


//#define MSG_DEBUG
//#define TCP_SEND_DEBUG
#define DEBUG
#define TCP_RECV_DEBUG
//#define RESEND_ZB
#define SCENE_DEBUG
#define SEND_LIST_DEBUG

#define calloc(s) malloc(s)
#define BUF_MSGPACK 2048

#define BRIGHTNESS     1
#define COLORTEMP      1 << 1
#define R_G_B 	       1 << 2

#define MAC_LEN    17  //mac地址长度
#define MAC_offset 6

#define MAX_LAMP_SEND_LIGHT  20
#define MAX_UART_SEND_LEN    90

#define TYPE_CCT 	1
#define TYPE_RGB 	2
#define TYPE_MOTOR  10
#define TYPE_PAD 	101



#define Start			0
#define Finish  		1
#define Delete			2
#define FILE_Ctrl  "/mnt/ctrl.info"
#define FILE_SCENE "/mnt/scene.info"
#define Send_Scene_Sync_Times  5
#define Send_Lamp_Login_Num	   5

extern msgpack_sbuffer* g_pk_buffer;
extern msgpack_packer* g_pk;
extern msgpack_unpacker* g_unpk;
extern int sockfd_server;
extern int gw_sockfd_server;
extern unsigned char zigbee_tcp_connect;
extern pthread_mutex_t scene_file_mutex;
extern uint8 resend_cmd_flag;
extern long  recv_cmd_time;
extern uint8 resend_num;
extern uint8 tcp_num_rigester;
extern unsigned char check_control_flag_main;
extern unsigned char check_control_flag_minor;
extern unsigned char scene_info_flag;
extern unsigned char send_scene_info_count;


void fun_msgpack_init(void);
int read_mac(char *hwaddr);
void get_mac_str(char *source,char *target);
uint8 get_Mac_Hex(char *mac_source,uint8 *mac_target,uint8 len);
void send_lamp_loginout(unsigned char *temp_node_mac,uint8 type);
int tcp_recive_unpack(char *msg,int len);
void msg_LcpLinkInitRsp(msgpack_object* result);
void msg_LcpLampLoginRsp(msgpack_object* result);
void msg_LCP_LINK_KEEPALIVE_RSP(msgpack_object* result);
void msg_LcpSetBrightnessColorTemp(msgpack_object* result);
void msg_LcpSetRGB(msgpack_object* result);
void msg_LcpSetGroupLampInfo(msgpack_object* result);
void msg_LcpSetMotorParams(msgpack_object* result);


struct Lamp_Set_Answer
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char lamp_id[4];
	unsigned char brightness;
	unsigned char colortemp;	
	unsigned char lightness;
	unsigned char valu_r;
	unsigned char valu_g;
	unsigned char valu_b;	
	unsigned char msg_end;
};

struct LampInfo
{
	unsigned char mac[8];
	int brightness;
	int colortemp;	
	int lightness;
	int valu_r;
	int valu_g;
	int valu_b;	
};
//面板情景设置
struct Pad_Set_Scene
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char pad_id[4];
	unsigned char key;	//指的是具体哪个灯
	unsigned char msg_end;
};

//面板情景重新设置
struct Pad_Set_Scene_new
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char pad_id[4];
	unsigned char key;
	unsigned char Key_ser;
	unsigned char msg_end;
};



struct ID_DATA_Set_Scene
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char lamp_id[4];
	unsigned char key;
	unsigned char msg_end;
};

struct MSG_Lamp_Bind_Ctrl
{
	unsigned char msg_head;
	unsigned char msg_len;
	unsigned char msg_id;
	unsigned char lamp_id[4];//灯具id
	unsigned char ctrl_id[4];//控制器id
	unsigned char msg_end;
};

struct scene_rsp
{
    unsigned char msg_head;
    unsigned char msg_len;
    unsigned char msg_id;
    unsigned char lamp_id[4];
    unsigned char lamp_value_1[12];
    unsigned char lamp_value_2[12];
    unsigned char lamp_value_3[12];
    unsigned char lamp_value_4[12];
    unsigned char msg_end;
};
#endif
