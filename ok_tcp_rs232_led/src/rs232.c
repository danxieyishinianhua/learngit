#include <stdio.h>
#include <string.h>
#include "tcp_rs232.h"
#include "zigbeemsg.h"
#include "nodearray.h"
#include "msgpack.h"
#include "fun_msgpack.h"
#include "send_list.h"

extern int senddebugdataresponse(char *data, int len);
long g_lastcommandtime = 0;
unsigned char check_control_flag_main = 1;//主检查控制标志
unsigned char check_control_flag_minor = 1;//次检查控制标志
unsigned char send_control_info_count = 0;//发送控制信息计数
unsigned char Heart_Beat_Send_buf[MAX_UART_SEND_LEN];//90
unsigned char Heart_Beat_wait_num = 0;//心跳等待次数标志位
long  Heart_Beat_first_time;
unsigned char scene_info_flag = 1;//情景信息标志位
unsigned char send_scene_info_count = 0;//发送情景消息的次数
pthread_mutex_t scene_file_mutex = PTHREAD_MUTEX_INITIALIZER;


int getcooaddr(char *readbuf, int buflen, char *zigbeecooaddr)//获取coo节点的地址
{
	int i = 0, j = 0;
	for(i = 0; i < buflen; i++)
	{
		if(((readbuf[i] <= '9') && (readbuf[i] >= '0') ) || ((readbuf[i] <= 'F') && (readbuf[i] >= 'A') ))
		{
			zigbeecooaddr[j] = readbuf[i];
			j++;
			if((j == 2) || (j == 5) || (j == 8) || (j == 11) || (j == 14))//硬件地址如00:0c:29:13:7c:a5 
			{
				zigbeecooaddr[j] = ':';
				j++;
			}
		}	
	}
}

int InitZigbee(ZBNETWORKPARAM *pzbnetworkparam )
{//686E518C
	int res = 0, count = 0;
	char tempbuf[100];
	char readbuf[256];
	printf("InitZigbee Start!\r\n");

	char  zigbeecooaddr[32];
	memset(zigbeecooaddr, 0, sizeof(zigbeecooaddr));
	count = 0;
	sprintf(tempbuf, "AT+SHOWADDR");//获取本节点的 MAC 地址
	rs232_send(tempbuf, strlen(tempbuf));
	while(count < 10)
	{
		usleep(200000);
		memset(readbuf, 0, sizeof(readbuf));
		res = read(rs232_fd, readbuf, sizeof(readbuf));
		if(res > 0)
		{
			printf("serial read:%s\r\n", readbuf);
			getcooaddr(readbuf + 6, res, zigbeecooaddr);//获取硬件地址只要后6个字节
			memcpy(pzbnetworkparam->zigbeecooaddr, zigbeecooaddr, sizeof(zigbeecooaddr));
			printf("zigbeecooaddr=%s\r\n", zigbeecooaddr);
			break;
		}
		count++;
	}

	
	count = 0;
	sprintf(tempbuf, "AT+GETINFO");//获取设备的基本配置信息
	rs232_send(tempbuf, strlen(tempbuf));
		
	int panid_flag = 0,ch_flag = 0;
	while(count < 11)
	{
		usleep(200000);
		memset(readbuf, 0, sizeof(readbuf));
		res = read(rs232_fd, readbuf, sizeof(readbuf));
		if(res > 0)
		{
			printf("serial read:%s\r\n", readbuf);
			if (strstr(readbuf, pzbnetworkparam->panid) == NULL)//如果readbuf没有panid的子串
			{

				sprintf(tempbuf, "AT+SETPID=%s", pzbnetworkparam->panid);//设置网络 PANID 参数
				printf("%s\r\n", tempbuf);
				usleep(1000000);
				rs232_send(tempbuf, strlen(tempbuf));	
				count = 10;
			}
			else panid_flag = 1;//readbuf中有panid

			sprintf(tempbuf, "Ch=%02X", pzbnetworkparam->channel);//将获得到的信道号存入tempbuf
			if(strstr(readbuf, tempbuf) == NULL)
			{
				sprintf(tempbuf, "AT+SETCH=%02X", pzbnetworkparam->channel);
				printf("%s\r\n", tempbuf);
				usleep(1000000);
				rs232_send(tempbuf, strlen(tempbuf));//将tempbuf写入rs232_fd
				count = 10;
			}
			else ch_flag = 1;//readbuf中有channel

			if(  panid_flag && ch_flag)break;//rs232_fd中存在panid和channel就不需要设置,直接跳出循环
		}

		if(count = 10)//说明rs232_fd没有获取到panid和channel的信息
		{
			sprintf(tempbuf, "AT+GETINFO");//获取设备的基本配置信息
			usleep(1000000);
			rs232_send(tempbuf, strlen(tempbuf));
			count = 0;
		}
		count++;
	}
	
	printf("reboot zigbee\r\n");
	sprintf(tempbuf, "AT+RESET");//zigbee模块复位
	rs232_send(tempbuf, strlen(tempbuf));
	while(count < 10)
	{
		usleep(200000);
		memset(readbuf, 0, sizeof(readbuf));
		res = read(rs232_fd, readbuf, sizeof(readbuf));
		if(res > 0)
		{
			printf("serial read:%s\r\n",readbuf);
			if(strstr(readbuf, "Reset in 6 seconds..."))
				break;
		}
		count++;
	}		
	printf("InitZigbee complete!\r\n");
}
long getcurrenttime()
{
    time_t tt;
    tt = time(NULL);
	return (long)tt;
}
void printhex(uint8 *databuf, int datalen)
{
	int i;
	for(i = 0; i < datalen; i++)
	{
		printf("%02X ", databuf[i]);
	}
	printf("\r\n");
	return;
}

void RSP_ID_NET_HeartBeat(char *msg)//心跳响应函数 网关--->设备
{
	zbnode zb_vals;
	int newnodeflag = 0;//新的节点标志位
	char *p = msg;
	memset(&zb_vals, 0, sizeof(zbnode));
	zb_vals.zbnodelive_val = 0;
	//MAC 地址的高 4 字节固定为0x000D6F00
	zb_vals.macaddr[0] = 0x00;
	zb_vals.macaddr[1] = 0x0D;
	zb_vals.macaddr[2] = 0x6F;
	zb_vals.macaddr[3] = 0x00;
	memcpy((uint8 *)&zb_vals.macaddr[4], p + 3, 4);//copy mac addr 
	zb_vals.testtime = getcurrenttime();//获取当前时间
	zb_vals.heartbeatinterval = HEARTBEATINTERVAL;//5s
	if (isExistNode(zb_vals.macaddr) == FALSE)//判断节点是否存在
	{	
		zb_vals.loginflag = 1;//loginflag=1说明次数有一个灯连接上了
		zb_vals.lamp_type = *(p + 7);
		zb_vals.bringhtness = *(p + 8);
		zb_vals.colortemp = *(p + 9);
		zb_vals.lightness = *(p + 10);
		zb_vals.red = *(p + 11);
		zb_vals.green = *(p + 12);
		zb_vals.blue = *(p + 13);
		zb_vals.heartbeattime = getcurrenttime();
		addNewzbNode((void*)&zb_vals);
		check_control_flag_minor = 1;//次检查控制标志位
		newnodeflag = 1;//新的节点标志位置1
	}
	else
	{
		//updatezbnodelive_val(zb_vals.macaddr, ZB_ON_LINE);
	}
	
	char tempsendbuf[20] = {0};
	tempsendbuf[0] = 0x7E;//消息头
	tempsendbuf[1] = 0x08;//消息长度
	tempsendbuf[2] = 0x04;//消息id--心跳响应请求
	memcpy(tempsendbuf + 3, (uint8 *)&zb_vals.macaddr[4], 4);//灯的mac地址
	tempsendbuf[7] = 0x5E;//消息尾
	memcpy((char *)&Heart_Beat_Send_buf[Heart_Beat_wait_num * 8], tempsendbuf, 8);
	Heart_Beat_wait_num++;
	if(Heart_Beat_wait_num == (MAX_UART_SEND_LEN / 8))//11
	{
		msg_write_to_list(Heart_Beat_Send_buf, Heart_Beat_wait_num * 8);
		Heart_Beat_wait_num = 0;
	}
	Heart_Beat_first_time = getcurrenttime();
	if(newnodeflag == 1)
	{
		SaveZigbeeNode();//保存zigbee节点
	}
}

void Rsp_Msg_Lihgt_Status(char *msg,int msglen)////查询灯具状态如:运行时间 重启次数灯发给QT
{
	if(fd_A[0] > 0) tcp_send(fd_A[0], msg, msglen);
}

void Rsp_Msg_Set_Answer(char *msg)//灯具调节响应函数 设备--->网关  面板调节时不调用
{
    printf("******Rsp_Msg_Set_Answer!!!******\n");
	zbnode zb_vals;
	char *p = msg;
	if(resend_cmd_flag == 0)return;
	memset(&zb_vals, 0, sizeof(zbnode));
	zb_vals.zbnodelive_val = 0;
	//zb_vals.macaddr[0] = 0x00;
	//zb_vals.macaddr[1] = 0x0D;
	//zb_vals.macaddr[2] = 0x6F;
	//zb_vals.macaddr[3] = 0x00;
	memcpy((uint8 *)&zb_vals.macaddr[4], p + 3, 4);
	zb_vals.bringhtness = p[7];
	zb_vals.colortemp = p[8];
	zb_vals.lightness = p[9];
	zb_vals.red = p[10];
	zb_vals.green = p[11];
	zb_vals.blue = p[12];
	check_light_status_answer((char *)&zb_vals);
}

/*面板发送的消息由网关转接后将消息ID加1后发送给串口从而控制灯*/
void Msg_Pad_Data(char *msg)//面板请求函数 面板--->网关
{
	struct Pad_Set_Scene temp;
	memcpy((char *)&temp, msg, sizeof(temp));
	temp.msg_id = ID_DATA_Pad_set_Rqt;//面板情景响应id=11
	msg_write_to_list((char *)&temp, sizeof(temp));
	FILE * fp = fopen(FILE_Ctrl, "rb+");//读写打开一个二进制文件，只允许读写数据
	if(fp == NULL)
	{
		printf("file /mnt/ctrl_info is open error! \n");
	}
	else
	{
		unsigned char pad_info[12]  = {0};
		if(fread(&pad_info, 1, 12, fp) == 12)//读取文件头 【0】 控制器个数   【1】 是否同步完成
		{
			if(pad_info[0] != 0)//若pad_info[0]存储的控制器个数不为0 ，则继续检查
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//面板的个数
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)
					{
						if(memcmp(lamp_mac_info, temp.pad_id, 4) == 0)//比较mac地址是否正确
						{
							int j = 0;
							for(j = 0; j < lamp_mac_info[5]; j++)//每个面板下灯具的个数
							{
								set_lamp_scene((char *)&lamp_mac_info[12 + j*6 + 2], temp.key);
							}
							//设置了情景参数就需要重新发送
							resend_cmd_flag = 1;//重发命令标志位
							recv_cmd_time = getcurrenttime();//重发命令的时间
							resend_num = 0;//重发次数置0
						}						
					}
				}
			}
		}
		fclose(fp);
	}
}


void Msg_Pad_Data_New(char *msg)//控制器、面板请求函数
{
	struct Pad_Set_Scene send_scene;
	struct Pad_Set_Scene_new temp;
	memcpy((char *)&temp, msg, sizeof(temp));
	static unsigned char ctrl_mac[4], send_count;
	if((memcmp((char *)&temp.pad_id, ctrl_mac, 4) == 0) && temp.Key_ser == send_count)return;//temp.Key_ser代表触发次数

	memcpy(ctrl_mac, (char *)&temp.pad_id, 4);//拷贝mac地址放入ctrl_mac数组中
	send_count = temp.Key_ser;
	memcpy((char *)&send_scene, msg, sizeof(send_scene));
	send_scene.msg_len = 0x9;
	send_scene.msg_id  = ID_DATA_Pad_set_Rqt;//面板情景响应id
	send_scene.msg_end = 0x5E;
    usleep(10000);
	msg_write_to_list((char *)&send_scene, sizeof(send_scene));
	FILE * fp = fopen(FILE_Ctrl, "rb+");
	if(fp == NULL)
	{
		printf("file /mnt/ctrl_info is open error! \n");
	}
	else
	{
		unsigned char pad_info[12] = {0};
		if(fread(&pad_info, 1, 12, fp) == 12)//读取文件头 【0】 控制器个数 【1】 是否同步完成
		{
			if(pad_info[0] != 0)//若面板个数不为0 ，则继续检查
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//面板的个数
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)//读取文件信息
					{
						if(memcmp(lamp_mac_info, temp.pad_id, 4) == 0)//比较每个面板中的mac地址，前四个字节是mac地址
						{
							int j = 0;
							for(j = 0; j < lamp_mac_info[5]; j++)//lamp_mac_info[5]代表每个控制器下灯具个数
							{
								set_lamp_scene((char *)&lamp_mac_info[12 + j*6 + 2], temp.key);//从第14个字节开始存储数据
							}
							resend_cmd_flag = 1;//说明有灯没连接需要重新发送连接请求
							recv_cmd_time = getcurrenttime();//获取需要重新发送时的时间
							resend_num = 0;//重发次数清零
						}	
					}
				}
			}
		}
		fclose(fp);
	}
}


void MSG_SET_SCENE_RSP(char *msg)//灯具响应面板函数  设备--->面板
{
    printf("******MSG_SET_SCENE_RSP!!!******\n");
	struct ID_DATA_Set_Scene temp;
	memcpy((char *)&temp, msg, sizeof(temp));
	check_lamp_scene(temp.lamp_id, temp.key);
	return;	
}

void Lamp_Scene_Sync_Rsp(char *msg)//灯具情景模式同步响应 设备--->网关
{
    printf("******Lamp_Scene_Sync_Rsp!!!******\n");
	pthread_mutex_lock(&scene_file_mutex); 
	FILE * fp = fopen(FILE_SCENE, "rb+");
	if(fp == NULL)
	{
		printf("file /mnt/scene.info is open error! \n");
	}
	else
	{
		char read_temp[27] = {0}, scene_head_parm[27] = {0};
		fread(scene_head_parm, 1, 27, fp);
		if(scene_head_parm[0] == Finish);//是否全部同步完成
		else
		{
			int i = 0;
			for(i = 0; i < scene_head_parm[1]; i++)//当前存储的灯具个数
			{
				fread(read_temp, 1, 27, fp);
				if(read_temp[0] != Finish)//是否同步完成
				{
					if(memcmp(msg + 3, (char *)&read_temp[2], 4) == 0)
					{
						read_temp[0]  = Finish;
						read_temp[1]  = 0;//总重发次数不使用时就置为0
						read_temp[26] = 0;//当前重发次数清零
						fseek(fp, -27, SEEK_CUR);
						fwrite(read_temp, 1, 27, fp);
					}
				}
			}
		}
		fclose(fp);
	}
	pthread_mutex_unlock(&scene_file_mutex); 
}

void Bind_Ctrl_Rsp(char *msg)//灯具与控制器绑定响应
{
	struct MSG_Lamp_Bind_Ctrl temp;
	memcpy((char *)&temp, msg, sizeof(temp));

	FILE * fp = fopen(FILE_Ctrl, "rb+");
	if(fp == NULL)
	{
		printf("file /mnt/ctrl_info is open error! \n");
	}
	else
	{
		unsigned char pad_info[12] = {0};
		if(fread(&pad_info, 1, 12, fp) == 12)//读取文件头 【0】 控制器个数   【1】 是否同步完成
		{
			if(pad_info[0] != 0)//若控制器个数不为0,则继续检查
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//面板的个数
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)
					{
						printf("Bind_Ctrl_Rsp, pad bind lamps num:%d\n", lamp_mac_info[5]);//控制器下灯具个数
						if(memcmp(lamp_mac_info, (char *)&temp.ctrl_id, 4) == 0)//0-3：控制器MAC；
						{
							int j;
							if(lamp_mac_info[4] == Finish)continue;//4:该控制器下的lamp是否已经全部绑定完成
							for(j = 0; j < lamp_mac_info[5]; j++)//每个控制器下灯具个数
							{
								if(lamp_mac_info[j*6 + 12] == Finish)continue;
								if(memcmp((char *)&lamp_mac_info[j*6 + 12 + 2], (char *)&temp.lamp_id, 4) == 0)
								{
									lamp_mac_info[j*6 + 12] = Finish;
									lamp_mac_info[j*6 + 12 + 1] = 0;
									fseek(fp, -612, SEEK_CUR);
									fwrite(lamp_mac_info, 1, 612, fp);
								}
							}
						}
					}
				}
			}
		}
		fclose(fp);
	}
}

void Bind_Ctrl_Delete_Rsp(char *msg)//灯具与控制器解绑响应
{
	struct MSG_Lamp_Bind_Ctrl temp;
	memcpy((char *)&temp, msg, sizeof(temp));

	FILE * fp = fopen(FILE_Ctrl, "r+");
	if(fp == NULL)
	{
		printf("file /mnt/scene.info is open error! \n");
	}
	else
	{
		unsigned char pad_info[12] = {0};
		int ret = 0;
		if(fread(&pad_info, 1, 12, fp) == 12)
		{
			if(pad_info[0] != 0)//控制器不为0就继续检查
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//控制器个数
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)
					{
						if(lamp_mac_info[4] == Finish)continue;
						if(memcmp(lamp_mac_info, (char *)&temp.ctrl_id, 4) == 0)
						{
							ret = 1;
							break;
						}
					}
				}
				if(ret)
				{
					int j = 0, lamp_num = 0;
					lamp_num = lamp_mac_info[5];
					for(j = 0; j < lamp_num; j++)
					{
						if(lamp_mac_info[j*6 + 12] != Delete)continue;
						if(memcmp((char *)&lamp_mac_info[j*6 + 12 + 2], (char *)&temp.lamp_id, 4) == 0)
						{
							memcpy((char *)&lamp_mac_info[j*6 + 12], (char *)&lamp_mac_info[(lamp_num - 1)*6 + 12], 6);
							if(lamp_num != 0)lamp_mac_info[5]--;
							memset((char *)&lamp_mac_info[(lamp_num - 1)*6 + 12], 0, 6);
							fseek(fp, -612, SEEK_CUR);
							fwrite(lamp_mac_info, 1, 612, fp);
							break;
						}
					}
				}
			}
		}
		fclose(fp);
	}	
}


int uart_recv_unpack_7E(int len,char *msg)
{
	unsigned char msg_id = *(msg + 2);//消息ID号
	#ifdef DEBUG
	printf("*******uart_recv_unpack_7E()*******  msgid:%d******\n", msg_id);
    #endif
	switch(msg_id)
	{
		case ID_NET_HeartBeat:			RSP_ID_NET_HeartBeat(msg);break;//心跳请求函数 设备--->网关
		case ID_DATA_Pad:				Msg_Pad_Data(msg);break;//面板请求函数 面板--->网关
		case ID_DATA_Control_scene:		Msg_Pad_Data_New(msg);break;//控制器、面板请求 面板--->网关
		case MSG_SET_Answer:			Rsp_Msg_Set_Answer(msg);break;//灯具调节响应 设备--->网关
		case MSG_GET_STATUS_Answer:		Rsp_Msg_Lihgt_Status(msg, len);break;//查询灯具状态响应 设备--->网关
		case MSG_Lamp_Bind_Ctrl_Rsp:	Bind_Ctrl_Rsp(msg);break;//灯具与控制器绑定响应 设备--->网关
		case MSG_Lamp_Delete_Ctrl_Rsp:	Bind_Ctrl_Delete_Rsp(msg);break;//灯具与控制器解绑响应 设备--->网关
		case ID_DATA_Set_Scene_Rsp:		MSG_SET_SCENE_RSP(msg);break;//灯具响应面板情景请求 设备--->网关
		case MSG_Scene_Sync_Rsp:		Lamp_Scene_Sync_Rsp(msg);break;//情景模式同步响应 设备--->网关
		default: {if(fd_A[0] > 0) tcp_send(fd_A[0], msg, len);}break;//发给QT用于测试
	}
	return 1; 
}

void *rs232_recv(void *arg)
{
	int i, j;
	int iret, maxfd, res;
	fd_set rx_set, wk_set;
	struct timeval tv ;

	FD_ZERO(&rx_set);
	FD_SET(rs232_fd, &rx_set);//将rs232_fd放入集合rx_set中
	maxfd = rs232_fd + 1;
	uint8 databuf[4096];
	uint8 readbuf[4096];
	int databuflen = 0;
	int timeSec = 0;
	int timeUsec = 300000;
	tcflush(rs232_fd, TCIOFLUSH);//清空串口已有的数据
	while(1)
	{
		FD_ZERO(&wk_set);
		for (j = 0; j < maxfd; j++)
		{
			if (FD_ISSET(j, &rx_set))
			{
				FD_SET(j, &wk_set);
			}
		}
		tv.tv_sec  = timeSec;
		tv.tv_usec = timeUsec;

		iret = 0;
		i = 0;
		databuflen = 0;
		memset(readbuf, 0, sizeof(readbuf));
		memset(databuf, 0, sizeof(databuf));//清零
		iret = select(maxfd, &wk_set, NULL, NULL, &tv);//select在300ms时间内阻塞，超时时间之内有事件到来就返回了，否则在超时后不管怎样一定返回 

		if(iret > 0)
		{
			res = read(rs232_fd, readbuf, sizeof(readbuf));
			while((res > 0) && (databuflen < sizeof(databuf)))
			{
				memcpy(databuf + databuflen, readbuf, res);//将rs232_fd文件中的内容存放到databuf中
				databuflen += res;
				res = read(rs232_fd, readbuf, sizeof(readbuf));//紧接着读取未读到的内容存入readbuff
			}
			if(databuflen > 3)//暂时不考虑多包含在一起的情况
			{
				printf("receive serial data len>3:len=%d\r\n", databuflen);//打印消息
				printhex(databuf, databuflen);
				int  templen = 0;
				char dstmac[8];
				char *p = databuf;
				if(strstr(databuf, "Node Table") != NULL)
				{
					NodeTableHandle(databuf, databuflen);
					continue;
				}
				while(templen < databuflen)
				{
				    //QT连接上时会执行下面的语句
					if((p[0] == 0x2A) && (READFLAG == ((EMBERCMDPACKET*)p)->clusterid[0]))//邻居表查询clusterid[0]代表读或写标志
					{
						int nodelen = 6 * MAXNEIGHBORCOUNT;//6*12
						char nodebuf[6 * MAXNEIGHBORCOUNT];
				
						dstmac[0] = 0x00;//mac地址的高四字节固定为000D6F00
						dstmac[1] = 0x0D;
						dstmac[2] = 0x6f;
						dstmac[3] = 0x00;
						
						EMBERCMDPACKET *tempemberpacket = (EMBERCMDPACKET*)databuf;
						dstmac[4] = tempemberpacket->srtaddr[3];//000D6F00
						dstmac[5] = tempemberpacket->srtaddr[2];
						dstmac[6] = tempemberpacket->srtaddr[1];
						dstmac[7] = tempemberpacket->srtaddr[0];
						memcpy(nodebuf, (char*)(databuf + sizeof(EMBERCMDPACKET))+5+4, nodelen);
						printf("receive neighbor:\r\n");
						printhex(nodebuf, nodelen);
						updateneighbor(dstmac, nodebuf, nodelen);
						updatezbnodelive_val(dstmac, ZB_ON_LINE);
						senddebugdataresponse(databuf, databuflen);
						break;
					}			
					else if(p[0] == 0x7E)//头信息为0x7E就继续
					{
						//databuflen表示串口文件中的数据长度
						unsigned char msg_len = p[1];
						if(p[1] <= (databuflen - templen))//确保接受长度正确
						{
							if(p[msg_len - 1] == 0x5E)//解包时以0x7E开头和0x5E结尾的为一串消息
							{
								uart_recv_unpack_7E(msg_len, p);//串口接收到的数据进行解包
								p += (int)msg_len;
								templen += (int)msg_len;
							}
							else break;
						}
						else break;
					}
					else break;
				}
			}
	    }
	}
	memset(log_buf, '\0', sizeof(log_buf));
	sprintf(log_buf, "pthread rs232_recv() exit ERROR :errno=%d -> %s", errno, strerror(errno));
	log_to_file(__func__, log_buf, NULL);
	pthread_exit(0);
}


char rs232_send(char *data, int datalen)
{
	int ret;
	ret = write(rs232_fd, data, datalen);
	if (ret <= 0)
	{
	#ifdef DEBUG
		printf("write to %s error!\n", RS232_DEVICE);
	#endif
		log_to_file(__func__, "write to rs232", "  ERROR");
		return -1;
	}
}


void *rs232_write_list(void *arg)
{
    while(1)
	{
		while(msg_read_to_list())//如果串口文件中还有数据就一直循环读取，否则跳出循环
		{
			usleep(g_zbnetworkparam.cmdsendinterval);//延时命令发送的间隔时间200ms
		}
		usleep(200000);	
	}
	memset(log_buf, '\0', sizeof(log_buf));
	sprintf(log_buf, "pthread rs232_write_list() exit ERROR :errno=%d -> %s", errno, strerror(errno));
	log_to_file(__func__, log_buf, NULL);
	pthread_exit(0);
}

void *rs232_resend(void *arg)
{
	while(1)
	{
		if(resend_cmd_flag == 1)
		{
			if((getcurrenttime() - recv_cmd_time) > 2)
			{
				recv_cmd_time = getcurrenttime();
				if(re_send_light_cmd() <= 0)
				{
					resend_cmd_flag = 0;
					resend_num = 0;
				}
				else
				{
					resend_num++;
					if(resend_num >= 5)resend_cmd_flag = 0, resend_num = 0;//说明未连接上，不再连接了
				}
			}
		}
		else
		{
			if(Heart_Beat_wait_num != 0)
			{
				if(getcurrenttime() - Heart_Beat_first_time > 10)//有灯具心跳消息10s后网关响应回灯具(<11)
				{
					msg_write_to_list(Heart_Beat_Send_buf, Heart_Beat_wait_num * 8);
					Heart_Beat_wait_num = 0;
				}
			}
		}
		if(check_control_flag_main)//灯具控制同步，若检查完成，除非平台下发新的数据才能激活
		{
			if(check_control_flag_minor)//一开始灯都是同步好的，只有服务器更改灯的消息时，才会用到
			{
				if(send_control_info_count >= 5)//重发次数超过，则等待下次登录，激活重发
				{
					check_control_flag_minor = 0;
					send_control_info_count = 0;
				}
				send_control_info_count++;
				
				FILE * fp = fopen(FILE_Ctrl, "rb+");// 读写打开一个二进制文件，只允许读写数据。
				if(fp == NULL)
				{
					printf("/mnt/ctrl.info file is not exist!!!\n");
					if(access(FILE_Ctrl, F_OK) == 0)return;//F_OK 只判断是否存在
					else
					{
						fp = fopen(FILE_Ctrl, "wb+");//读写打开或建立一个二进制文件，允许读和写
						if(fp == NULL)
						{
							printf("file /mnt/ctrl.info is creat erro!!!\n");
							return;
						}
						else//创建文件成功
						{
							char str_buf[12] = {0};//用于保存文件信息
							str_buf[0] = 0;//控制器个数为0
							str_buf[1] = Finish;
							fwrite(str_buf, 1, 12, fp);//设置灯全部绑定完成
							fclose(fp);
							check_control_flag_minor = 0;//灯具同步完成
							send_control_info_count = 0;
						}
					}
				}
				else
				{
					/* 文件信息: 0：存储的控制器数据个数  1：控制器是否与lamp全部绑定完成  2-11：保留	*/
					unsigned char pad_info[12] = {0};
					if(fread(&pad_info, 1, 12, fp) == 12)//读取文件头 【0】 控制器个数   【1】 是否同步完成
					{
						#if 0
						printf("/mnt/ctrl_info pad_num:%d, pad_flag:%d\n", pad_info[0], pad_info[1]);
						#endif
						if(pad_info[1] == Finish)//若同步完成，则关闭同步程序
						{
							check_control_flag_main = 0;
							check_control_flag_minor = 0;
						}
						else//没有同步完成
						{
							unsigned char ret = 1;
							if(pad_info[0] != 0)//若控制器个数不为0 ，则继续检查
							{
								unsigned char send_num = 0;
								unsigned char send_buf[MAX_UART_SEND_LEN];
								send_buf[0] = 0X7E;
								send_buf[2] = MSG_Lamp_Bind_Ctrl;//50
								unsigned char lamp_mac_info[612] = {0};
								int i;
								for(i = 0; i < pad_info[0]; i++)//pad_info[0]代表控制器的个数
								{
									if(fread(lamp_mac_info, 1, 612, fp) == 612)//
									{
										if(lamp_mac_info[4] == Finish)continue;//说明该控制器下lamp全部绑定完成就判断下一个
										int j;
										unsigned char is_ctrl_finish = 1;
										for(j = 0; j < lamp_mac_info[5]; j++)//lamp_mac_info[5]代表控制器下灯具个数
										{
											if(lamp_mac_info[12 + j*6] == Finish)continue;//灯具绑定完成就判断下一个灯
											else if(lamp_mac_info[12 + j*6] == Delete)//解绑灯具信息
											{
												unsigned char str[20] = {0};
												str[0] = 0x7E;
												str[1] = 0x0C;
												str[2] = MSG_Lamp_Delete_Ctrl;//解除绑定的消息ID
												memcpy((char *)&str[3], (char *)&lamp_mac_info[12 + j*6 + 2], 4);//14~17灯的MAC地址
												memcpy((char *)&str[7], (char *)&lamp_mac_info, 4);//0~3控制器的MAC地址
												str[11] = 0x5E;	
												msg_write_to_list(str, 12);//发送给单片机去解绑该控制器下的灯
												ret = 0;//解绑完成将标志位ret置0
												is_ctrl_finish = 0;//解绑完成将标志位is_ctrl_finish置0
												continue;
											}
											else//等待绑定
											{
												ret = 0;
												is_ctrl_finish = 0;
												memcpy((char *)&send_buf[send_num * 8 + 4], (char *)&lamp_mac_info[12 + j * 6 + 2], 4);//灯MAC地址
												memcpy((char *)&send_buf[send_num * 8 + 8], (char *)&lamp_mac_info, 4);//控制器MAC地址
												send_num++;
												if(send_num == ((MAX_UART_SEND_LEN - 10) / 8))
												{
													send_buf[3] = send_num;
													send_buf[1] = send_num * 8 + 5;
													send_buf[send_num * 8 + 4] = 0x5E;
													send_num = 0;
													msg_write_to_list(send_buf, send_buf[1]);
												}
											}	
										}
										if(is_ctrl_finish == 1)//灯具全部绑定了
										{
											lamp_mac_info[4] = Finish;//标志该控制器的所有灯全部同步完成
											fseek(fp, -612, SEEK_CUR);
											fwrite(lamp_mac_info, 1, 612, fp);
										}
									}
								}
								if(send_num != 0)//绑定send_num个灯
								{
									send_buf[3] = send_num;
									send_buf[1] = send_num * 8 + 5;
									send_buf[send_num * 8 + 4] = 0x5E;
									msg_write_to_list(send_buf, send_buf[1]);//发送给单片机让灯具绑定该控制器
								}
							}
							else//控制器个数为0
							{
								check_control_flag_main = 0;
							}
							if(ret == 1)//已经绑定完成了或没有控制器
							{
								fseek(fp, 1, SEEK_SET);
								unsigned char writebuf[2];
								writebuf[0] = 1;//控制器与lamp全部绑定完成
								fwrite(writebuf, 1, 1, fp);
								check_control_flag_main = 0;
							}
						}
					}
					fclose(fp);
				}
			}
		}
		if(scene_info_flag)//情景模式 LcpSyncDefaultSceneInfo函数中scene_info_flag=1
		{
			pthread_mutex_lock(&scene_file_mutex); 
			FILE * fp = fopen(FILE_SCENE, "rb+");// 读写打开一个二进制文件，只允许读写数据。
			if(fp == NULL)
			{
				printf("file /mnt/scene.info is not exist! \n");
				if(access(FILE_SCENE, F_OK) == 0)return;//return是添加的
				else
				{
					fp = fopen(FILE_SCENE, "wb+");//读写打开或建立一个二进制文件，允许读和写
					if(fp == NULL)
					{
						printf("file /mnt/scene.info is creat error! \n");
					}
					else
					{
						char str_buf[27] = {0};
						str_buf[0] = Finish;//同步完成
						str_buf[1] = 0;//灯具个数为0
						fwrite(str_buf, 1, 27, fp);//将信息写入FILE_SCENE文件中
						fclose(fp);
					}
				}
			}
			else//该文件打开成功
			{
				char read_temp[27], scene_head_parm[27];
				fread(scene_head_parm, 1, 27, fp);//读取情景文件的文件头信息
				int i, ret = 1, ret_1 = 1;
				static int status_scene = 0;//情景状态标志位
				if(scene_head_parm[0] == Finish)//scene_head_parm[0]代表是否同步完成
				{
					scene_info_flag = 0;
					status_scene = 1;//情景状态标志位置为1
				}
				else//未同步完成
				{
					for(i = 0; i < scene_head_parm[1]; i++)//当前存储的灯具个数
					{
						fread(read_temp, 1, 27, fp);//再读取后27个字节
						if(read_temp[0] != Finish)//没有同步完成
						{
							char mac_scene[8];
							ret = 0;
							if(status_scene == 0)
							{
								read_temp[1] = 0;//总重发次数清0
								fseek(fp, -27, SEEK_CUR);
								fwrite(read_temp, 1, 27, fp);
								continue;
							}
							if(read_temp[1] != send_scene_info_count)continue;
							memcpy((char *)&mac_scene[4], (char *)&read_temp[2], 4);//3~6是MAC地址
							if (isExistNode(mac_scene) == FALSE)
							{
								struct scene_rsp temp;
								temp.msg_head = 0x7E;
								temp.msg_len = sizeof(struct scene_rsp);
								temp.msg_id = MSG_Scene_Sync;//灯具情景模式同步消息
								memcpy(temp.lamp_id, (char *)&read_temp[2], 4);//将MAC地址存入lamp_id中
								temp.lamp_value_1[0] = read_temp[6];  temp.lamp_value_1[1] = read_temp[7];  temp.lamp_value_1[2] = read_temp[6];
								temp.lamp_value_1[3] = read_temp[8];  temp.lamp_value_1[4] = read_temp[9];  temp.lamp_value_1[5] = read_temp[10];
								temp.lamp_value_2[0] = read_temp[11]; temp.lamp_value_2[1] = read_temp[12]; temp.lamp_value_2[2] = read_temp[11];
								temp.lamp_value_2[3] = read_temp[13]; temp.lamp_value_2[4] = read_temp[14]; temp.lamp_value_2[5] = read_temp[15];
								temp.lamp_value_3[0] = read_temp[16]; temp.lamp_value_3[1] = read_temp[17]; temp.lamp_value_3[2] = read_temp[16];
								temp.lamp_value_3[3] = read_temp[18]; temp.lamp_value_3[4] = read_temp[19]; temp.lamp_value_3[5] = read_temp[20];
								temp.lamp_value_4[0] = read_temp[21]; temp.lamp_value_4[1] = read_temp[22]; temp.lamp_value_4[2] = read_temp[21];
								temp.lamp_value_4[3] = read_temp[23]; temp.lamp_value_4[4] = read_temp[24]; temp.lamp_value_4[5] = read_temp[25];
								temp.msg_end = 0x5E;
								msg_write_to_list((char *)&temp, sizeof(struct scene_rsp));

								read_temp[1]++;
								fseek(fp, -27, SEEK_CUR);
								fwrite(read_temp, 1, 27, fp);

								ret_1 = 0;
								break;
							}
						}
					}

					if(ret)//同步完成
					{
						fseek(fp, 0, SEEK_SET);//定位到文件开头
						unsigned char writebuf[2];
						writebuf[0] = 1;
						fwrite(writebuf, 1, 1, fp);//只有第一位为1,其它清空
						scene_info_flag = 0;
					}
					if(ret_1)//如果灯的MAC地址都匹配得上就继续执行
					{
						send_scene_info_count++;
						if(send_scene_info_count == Send_Scene_Sync_Times)//停止连接
						{	
							scene_info_flag = 0;
							send_scene_info_count = 0;//清零
						}
					}
					if(status_scene == 0)status_scene = 1;//没有同步成功
				}
				fclose(fp);
			}
			pthread_mutex_unlock(&scene_file_mutex); 
		}
		usleep(1000000);
	}
	pthread_exit(0);
}


