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
unsigned char check_control_flag_main = 1;//�������Ʊ�־
unsigned char check_control_flag_minor = 1;//�μ����Ʊ�־
unsigned char send_control_info_count = 0;//���Ϳ�����Ϣ����
unsigned char Heart_Beat_Send_buf[MAX_UART_SEND_LEN];//90
unsigned char Heart_Beat_wait_num = 0;//�����ȴ�������־λ
long  Heart_Beat_first_time;
unsigned char scene_info_flag = 1;//�龰��Ϣ��־λ
unsigned char send_scene_info_count = 0;//�����龰��Ϣ�Ĵ���
pthread_mutex_t scene_file_mutex = PTHREAD_MUTEX_INITIALIZER;


int getcooaddr(char *readbuf, int buflen, char *zigbeecooaddr)//��ȡcoo�ڵ�ĵ�ַ
{
	int i = 0, j = 0;
	for(i = 0; i < buflen; i++)
	{
		if(((readbuf[i] <= '9') && (readbuf[i] >= '0') ) || ((readbuf[i] <= 'F') && (readbuf[i] >= 'A') ))
		{
			zigbeecooaddr[j] = readbuf[i];
			j++;
			if((j == 2) || (j == 5) || (j == 8) || (j == 11) || (j == 14))//Ӳ����ַ��00:0c:29:13:7c:a5 
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
	sprintf(tempbuf, "AT+SHOWADDR");//��ȡ���ڵ�� MAC ��ַ
	rs232_send(tempbuf, strlen(tempbuf));
	while(count < 10)
	{
		usleep(200000);
		memset(readbuf, 0, sizeof(readbuf));
		res = read(rs232_fd, readbuf, sizeof(readbuf));
		if(res > 0)
		{
			printf("serial read:%s\r\n", readbuf);
			getcooaddr(readbuf + 6, res, zigbeecooaddr);//��ȡӲ����ַֻҪ��6���ֽ�
			memcpy(pzbnetworkparam->zigbeecooaddr, zigbeecooaddr, sizeof(zigbeecooaddr));
			printf("zigbeecooaddr=%s\r\n", zigbeecooaddr);
			break;
		}
		count++;
	}

	
	count = 0;
	sprintf(tempbuf, "AT+GETINFO");//��ȡ�豸�Ļ���������Ϣ
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
			if (strstr(readbuf, pzbnetworkparam->panid) == NULL)//���readbufû��panid���Ӵ�
			{

				sprintf(tempbuf, "AT+SETPID=%s", pzbnetworkparam->panid);//�������� PANID ����
				printf("%s\r\n", tempbuf);
				usleep(1000000);
				rs232_send(tempbuf, strlen(tempbuf));	
				count = 10;
			}
			else panid_flag = 1;//readbuf����panid

			sprintf(tempbuf, "Ch=%02X", pzbnetworkparam->channel);//����õ����ŵ��Ŵ���tempbuf
			if(strstr(readbuf, tempbuf) == NULL)
			{
				sprintf(tempbuf, "AT+SETCH=%02X", pzbnetworkparam->channel);
				printf("%s\r\n", tempbuf);
				usleep(1000000);
				rs232_send(tempbuf, strlen(tempbuf));//��tempbufд��rs232_fd
				count = 10;
			}
			else ch_flag = 1;//readbuf����channel

			if(  panid_flag && ch_flag)break;//rs232_fd�д���panid��channel�Ͳ���Ҫ����,ֱ������ѭ��
		}

		if(count = 10)//˵��rs232_fdû�л�ȡ��panid��channel����Ϣ
		{
			sprintf(tempbuf, "AT+GETINFO");//��ȡ�豸�Ļ���������Ϣ
			usleep(1000000);
			rs232_send(tempbuf, strlen(tempbuf));
			count = 0;
		}
		count++;
	}
	
	printf("reboot zigbee\r\n");
	sprintf(tempbuf, "AT+RESET");//zigbeeģ�鸴λ
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

void RSP_ID_NET_HeartBeat(char *msg)//������Ӧ���� ����--->�豸
{
	zbnode zb_vals;
	int newnodeflag = 0;//�µĽڵ��־λ
	char *p = msg;
	memset(&zb_vals, 0, sizeof(zbnode));
	zb_vals.zbnodelive_val = 0;
	//MAC ��ַ�ĸ� 4 �ֽڹ̶�Ϊ0x000D6F00
	zb_vals.macaddr[0] = 0x00;
	zb_vals.macaddr[1] = 0x0D;
	zb_vals.macaddr[2] = 0x6F;
	zb_vals.macaddr[3] = 0x00;
	memcpy((uint8 *)&zb_vals.macaddr[4], p + 3, 4);//copy mac addr 
	zb_vals.testtime = getcurrenttime();//��ȡ��ǰʱ��
	zb_vals.heartbeatinterval = HEARTBEATINTERVAL;//5s
	if (isExistNode(zb_vals.macaddr) == FALSE)//�жϽڵ��Ƿ����
	{	
		zb_vals.loginflag = 1;//loginflag=1˵��������һ������������
		zb_vals.lamp_type = *(p + 7);
		zb_vals.bringhtness = *(p + 8);
		zb_vals.colortemp = *(p + 9);
		zb_vals.lightness = *(p + 10);
		zb_vals.red = *(p + 11);
		zb_vals.green = *(p + 12);
		zb_vals.blue = *(p + 13);
		zb_vals.heartbeattime = getcurrenttime();
		addNewzbNode((void*)&zb_vals);
		check_control_flag_minor = 1;//�μ����Ʊ�־λ
		newnodeflag = 1;//�µĽڵ��־λ��1
	}
	else
	{
		//updatezbnodelive_val(zb_vals.macaddr, ZB_ON_LINE);
	}
	
	char tempsendbuf[20] = {0};
	tempsendbuf[0] = 0x7E;//��Ϣͷ
	tempsendbuf[1] = 0x08;//��Ϣ����
	tempsendbuf[2] = 0x04;//��Ϣid--������Ӧ����
	memcpy(tempsendbuf + 3, (uint8 *)&zb_vals.macaddr[4], 4);//�Ƶ�mac��ַ
	tempsendbuf[7] = 0x5E;//��Ϣβ
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
		SaveZigbeeNode();//����zigbee�ڵ�
	}
}

void Rsp_Msg_Lihgt_Status(char *msg,int msglen)////��ѯ�ƾ�״̬��:����ʱ�� ���������Ʒ���QT
{
	if(fd_A[0] > 0) tcp_send(fd_A[0], msg, msglen);
}

void Rsp_Msg_Set_Answer(char *msg)//�ƾߵ�����Ӧ���� �豸--->����  ������ʱ������
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

/*��巢�͵���Ϣ������ת�Ӻ���ϢID��1���͸����ڴӶ����Ƶ�*/
void Msg_Pad_Data(char *msg)//��������� ���--->����
{
	struct Pad_Set_Scene temp;
	memcpy((char *)&temp, msg, sizeof(temp));
	temp.msg_id = ID_DATA_Pad_set_Rqt;//����龰��Ӧid=11
	msg_write_to_list((char *)&temp, sizeof(temp));
	FILE * fp = fopen(FILE_Ctrl, "rb+");//��д��һ���������ļ���ֻ�����д����
	if(fp == NULL)
	{
		printf("file /mnt/ctrl_info is open error! \n");
	}
	else
	{
		unsigned char pad_info[12]  = {0};
		if(fread(&pad_info, 1, 12, fp) == 12)//��ȡ�ļ�ͷ ��0�� ����������   ��1�� �Ƿ�ͬ�����
		{
			if(pad_info[0] != 0)//��pad_info[0]�洢�Ŀ�����������Ϊ0 ����������
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//���ĸ���
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)
					{
						if(memcmp(lamp_mac_info, temp.pad_id, 4) == 0)//�Ƚ�mac��ַ�Ƿ���ȷ
						{
							int j = 0;
							for(j = 0; j < lamp_mac_info[5]; j++)//ÿ������µƾߵĸ���
							{
								set_lamp_scene((char *)&lamp_mac_info[12 + j*6 + 2], temp.key);
							}
							//�������龰��������Ҫ���·���
							resend_cmd_flag = 1;//�ط������־λ
							recv_cmd_time = getcurrenttime();//�ط������ʱ��
							resend_num = 0;//�ط�������0
						}						
					}
				}
			}
		}
		fclose(fp);
	}
}


void Msg_Pad_Data_New(char *msg)//�����������������
{
	struct Pad_Set_Scene send_scene;
	struct Pad_Set_Scene_new temp;
	memcpy((char *)&temp, msg, sizeof(temp));
	static unsigned char ctrl_mac[4], send_count;
	if((memcmp((char *)&temp.pad_id, ctrl_mac, 4) == 0) && temp.Key_ser == send_count)return;//temp.Key_ser����������

	memcpy(ctrl_mac, (char *)&temp.pad_id, 4);//����mac��ַ����ctrl_mac������
	send_count = temp.Key_ser;
	memcpy((char *)&send_scene, msg, sizeof(send_scene));
	send_scene.msg_len = 0x9;
	send_scene.msg_id  = ID_DATA_Pad_set_Rqt;//����龰��Ӧid
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
		if(fread(&pad_info, 1, 12, fp) == 12)//��ȡ�ļ�ͷ ��0�� ���������� ��1�� �Ƿ�ͬ�����
		{
			if(pad_info[0] != 0)//����������Ϊ0 ����������
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//���ĸ���
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)//��ȡ�ļ���Ϣ
					{
						if(memcmp(lamp_mac_info, temp.pad_id, 4) == 0)//�Ƚ�ÿ������е�mac��ַ��ǰ�ĸ��ֽ���mac��ַ
						{
							int j = 0;
							for(j = 0; j < lamp_mac_info[5]; j++)//lamp_mac_info[5]����ÿ���������µƾ߸���
							{
								set_lamp_scene((char *)&lamp_mac_info[12 + j*6 + 2], temp.key);//�ӵ�14���ֽڿ�ʼ�洢����
							}
							resend_cmd_flag = 1;//˵���е�û������Ҫ���·�����������
							recv_cmd_time = getcurrenttime();//��ȡ��Ҫ���·���ʱ��ʱ��
							resend_num = 0;//�ط���������
						}	
					}
				}
			}
		}
		fclose(fp);
	}
}


void MSG_SET_SCENE_RSP(char *msg)//�ƾ���Ӧ��庯��  �豸--->���
{
    printf("******MSG_SET_SCENE_RSP!!!******\n");
	struct ID_DATA_Set_Scene temp;
	memcpy((char *)&temp, msg, sizeof(temp));
	check_lamp_scene(temp.lamp_id, temp.key);
	return;	
}

void Lamp_Scene_Sync_Rsp(char *msg)//�ƾ��龰ģʽͬ����Ӧ �豸--->����
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
		if(scene_head_parm[0] == Finish);//�Ƿ�ȫ��ͬ�����
		else
		{
			int i = 0;
			for(i = 0; i < scene_head_parm[1]; i++)//��ǰ�洢�ĵƾ߸���
			{
				fread(read_temp, 1, 27, fp);
				if(read_temp[0] != Finish)//�Ƿ�ͬ�����
				{
					if(memcmp(msg + 3, (char *)&read_temp[2], 4) == 0)
					{
						read_temp[0]  = Finish;
						read_temp[1]  = 0;//���ط�������ʹ��ʱ����Ϊ0
						read_temp[26] = 0;//��ǰ�ط���������
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

void Bind_Ctrl_Rsp(char *msg)//�ƾ������������Ӧ
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
		if(fread(&pad_info, 1, 12, fp) == 12)//��ȡ�ļ�ͷ ��0�� ����������   ��1�� �Ƿ�ͬ�����
		{
			if(pad_info[0] != 0)//��������������Ϊ0,��������
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//���ĸ���
				{
					if(fread(lamp_mac_info, 1, 612, fp) == 612)
					{
						printf("Bind_Ctrl_Rsp, pad bind lamps num:%d\n", lamp_mac_info[5]);//�������µƾ߸���
						if(memcmp(lamp_mac_info, (char *)&temp.ctrl_id, 4) == 0)//0-3��������MAC��
						{
							int j;
							if(lamp_mac_info[4] == Finish)continue;//4:�ÿ������µ�lamp�Ƿ��Ѿ�ȫ�������
							for(j = 0; j < lamp_mac_info[5]; j++)//ÿ���������µƾ߸���
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

void Bind_Ctrl_Delete_Rsp(char *msg)//�ƾ�������������Ӧ
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
			if(pad_info[0] != 0)//��������Ϊ0�ͼ������
			{
				unsigned char lamp_mac_info[612] = {0};
				int i = 0;
				for(i = 0; i < pad_info[0]; i++)//����������
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
	unsigned char msg_id = *(msg + 2);//��ϢID��
	#ifdef DEBUG
	printf("*******uart_recv_unpack_7E()*******  msgid:%d******\n", msg_id);
    #endif
	switch(msg_id)
	{
		case ID_NET_HeartBeat:			RSP_ID_NET_HeartBeat(msg);break;//���������� �豸--->����
		case ID_DATA_Pad:				Msg_Pad_Data(msg);break;//��������� ���--->����
		case ID_DATA_Control_scene:		Msg_Pad_Data_New(msg);break;//��������������� ���--->����
		case MSG_SET_Answer:			Rsp_Msg_Set_Answer(msg);break;//�ƾߵ�����Ӧ �豸--->����
		case MSG_GET_STATUS_Answer:		Rsp_Msg_Lihgt_Status(msg, len);break;//��ѯ�ƾ�״̬��Ӧ �豸--->����
		case MSG_Lamp_Bind_Ctrl_Rsp:	Bind_Ctrl_Rsp(msg);break;//�ƾ������������Ӧ �豸--->����
		case MSG_Lamp_Delete_Ctrl_Rsp:	Bind_Ctrl_Delete_Rsp(msg);break;//�ƾ�������������Ӧ �豸--->����
		case ID_DATA_Set_Scene_Rsp:		MSG_SET_SCENE_RSP(msg);break;//�ƾ���Ӧ����龰���� �豸--->����
		case MSG_Scene_Sync_Rsp:		Lamp_Scene_Sync_Rsp(msg);break;//�龰ģʽͬ����Ӧ �豸--->����
		default: {if(fd_A[0] > 0) tcp_send(fd_A[0], msg, len);}break;//����QT���ڲ���
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
	FD_SET(rs232_fd, &rx_set);//��rs232_fd���뼯��rx_set��
	maxfd = rs232_fd + 1;
	uint8 databuf[4096];
	uint8 readbuf[4096];
	int databuflen = 0;
	int timeSec = 0;
	int timeUsec = 300000;
	tcflush(rs232_fd, TCIOFLUSH);//��մ������е�����
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
		memset(databuf, 0, sizeof(databuf));//����
		iret = select(maxfd, &wk_set, NULL, NULL, &tv);//select��300msʱ������������ʱʱ��֮�����¼������ͷ����ˣ������ڳ�ʱ�󲻹�����һ������ 

		if(iret > 0)
		{
			res = read(rs232_fd, readbuf, sizeof(readbuf));
			while((res > 0) && (databuflen < sizeof(databuf)))
			{
				memcpy(databuf + databuflen, readbuf, res);//��rs232_fd�ļ��е����ݴ�ŵ�databuf��
				databuflen += res;
				res = read(rs232_fd, readbuf, sizeof(readbuf));//�����Ŷ�ȡδ���������ݴ���readbuff
			}
			if(databuflen > 3)//��ʱ�����Ƕ������һ������
			{
				printf("receive serial data len>3:len=%d\r\n", databuflen);//��ӡ��Ϣ
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
				    //QT������ʱ��ִ����������
					if((p[0] == 0x2A) && (READFLAG == ((EMBERCMDPACKET*)p)->clusterid[0]))//�ھӱ��ѯclusterid[0]�������д��־
					{
						int nodelen = 6 * MAXNEIGHBORCOUNT;//6*12
						char nodebuf[6 * MAXNEIGHBORCOUNT];
				
						dstmac[0] = 0x00;//mac��ַ�ĸ����ֽڹ̶�Ϊ000D6F00
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
					else if(p[0] == 0x7E)//ͷ��ϢΪ0x7E�ͼ���
					{
						//databuflen��ʾ�����ļ��е����ݳ���
						unsigned char msg_len = p[1];
						if(p[1] <= (databuflen - templen))//ȷ�����ܳ�����ȷ
						{
							if(p[msg_len - 1] == 0x5E)//���ʱ��0x7E��ͷ��0x5E��β��Ϊһ����Ϣ
							{
								uart_recv_unpack_7E(msg_len, p);//���ڽ��յ������ݽ��н��
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
		while(msg_read_to_list())//��������ļ��л������ݾ�һֱѭ����ȡ����������ѭ��
		{
			usleep(g_zbnetworkparam.cmdsendinterval);//��ʱ����͵ļ��ʱ��200ms
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
					if(resend_num >= 5)resend_cmd_flag = 0, resend_num = 0;//˵��δ�����ϣ�����������
				}
			}
		}
		else
		{
			if(Heart_Beat_wait_num != 0)
			{
				if(getcurrenttime() - Heart_Beat_first_time > 10)//�еƾ�������Ϣ10s��������Ӧ�صƾ�(<11)
				{
					msg_write_to_list(Heart_Beat_Send_buf, Heart_Beat_wait_num * 8);
					Heart_Beat_wait_num = 0;
				}
			}
		}
		if(check_control_flag_main)//�ƾ߿���ͬ�����������ɣ�����ƽ̨�·��µ����ݲ��ܼ���
		{
			if(check_control_flag_minor)//һ��ʼ�ƶ���ͬ���õģ�ֻ�з��������ĵƵ���Ϣʱ���Ż��õ�
			{
				if(send_control_info_count >= 5)//�ط�������������ȴ��´ε�¼�������ط�
				{
					check_control_flag_minor = 0;
					send_control_info_count = 0;
				}
				send_control_info_count++;
				
				FILE * fp = fopen(FILE_Ctrl, "rb+");// ��д��һ���������ļ���ֻ�����д���ݡ�
				if(fp == NULL)
				{
					printf("/mnt/ctrl.info file is not exist!!!\n");
					if(access(FILE_Ctrl, F_OK) == 0)return;//F_OK ֻ�ж��Ƿ����
					else
					{
						fp = fopen(FILE_Ctrl, "wb+");//��д�򿪻���һ���������ļ����������д
						if(fp == NULL)
						{
							printf("file /mnt/ctrl.info is creat erro!!!\n");
							return;
						}
						else//�����ļ��ɹ�
						{
							char str_buf[12] = {0};//���ڱ����ļ���Ϣ
							str_buf[0] = 0;//����������Ϊ0
							str_buf[1] = Finish;
							fwrite(str_buf, 1, 12, fp);//���õ�ȫ�������
							fclose(fp);
							check_control_flag_minor = 0;//�ƾ�ͬ�����
							send_control_info_count = 0;
						}
					}
				}
				else
				{
					/* �ļ���Ϣ: 0���洢�Ŀ��������ݸ���  1���������Ƿ���lampȫ�������  2-11������	*/
					unsigned char pad_info[12] = {0};
					if(fread(&pad_info, 1, 12, fp) == 12)//��ȡ�ļ�ͷ ��0�� ����������   ��1�� �Ƿ�ͬ�����
					{
						#if 0
						printf("/mnt/ctrl_info pad_num:%d, pad_flag:%d\n", pad_info[0], pad_info[1]);
						#endif
						if(pad_info[1] == Finish)//��ͬ����ɣ���ر�ͬ������
						{
							check_control_flag_main = 0;
							check_control_flag_minor = 0;
						}
						else//û��ͬ�����
						{
							unsigned char ret = 1;
							if(pad_info[0] != 0)//��������������Ϊ0 ����������
							{
								unsigned char send_num = 0;
								unsigned char send_buf[MAX_UART_SEND_LEN];
								send_buf[0] = 0X7E;
								send_buf[2] = MSG_Lamp_Bind_Ctrl;//50
								unsigned char lamp_mac_info[612] = {0};
								int i;
								for(i = 0; i < pad_info[0]; i++)//pad_info[0]����������ĸ���
								{
									if(fread(lamp_mac_info, 1, 612, fp) == 612)//
									{
										if(lamp_mac_info[4] == Finish)continue;//˵���ÿ�������lampȫ������ɾ��ж���һ��
										int j;
										unsigned char is_ctrl_finish = 1;
										for(j = 0; j < lamp_mac_info[5]; j++)//lamp_mac_info[5]����������µƾ߸���
										{
											if(lamp_mac_info[12 + j*6] == Finish)continue;//�ƾ߰���ɾ��ж���һ����
											else if(lamp_mac_info[12 + j*6] == Delete)//���ƾ���Ϣ
											{
												unsigned char str[20] = {0};
												str[0] = 0x7E;
												str[1] = 0x0C;
												str[2] = MSG_Lamp_Delete_Ctrl;//����󶨵���ϢID
												memcpy((char *)&str[3], (char *)&lamp_mac_info[12 + j*6 + 2], 4);//14~17�Ƶ�MAC��ַ
												memcpy((char *)&str[7], (char *)&lamp_mac_info, 4);//0~3��������MAC��ַ
												str[11] = 0x5E;	
												msg_write_to_list(str, 12);//���͸���Ƭ��ȥ���ÿ������µĵ�
												ret = 0;//�����ɽ���־λret��0
												is_ctrl_finish = 0;//�����ɽ���־λis_ctrl_finish��0
												continue;
											}
											else//�ȴ���
											{
												ret = 0;
												is_ctrl_finish = 0;
												memcpy((char *)&send_buf[send_num * 8 + 4], (char *)&lamp_mac_info[12 + j * 6 + 2], 4);//��MAC��ַ
												memcpy((char *)&send_buf[send_num * 8 + 8], (char *)&lamp_mac_info, 4);//������MAC��ַ
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
										if(is_ctrl_finish == 1)//�ƾ�ȫ������
										{
											lamp_mac_info[4] = Finish;//��־�ÿ����������е�ȫ��ͬ�����
											fseek(fp, -612, SEEK_CUR);
											fwrite(lamp_mac_info, 1, 612, fp);
										}
									}
								}
								if(send_num != 0)//��send_num����
								{
									send_buf[3] = send_num;
									send_buf[1] = send_num * 8 + 5;
									send_buf[send_num * 8 + 4] = 0x5E;
									msg_write_to_list(send_buf, send_buf[1]);//���͸���Ƭ���õƾ߰󶨸ÿ�����
								}
							}
							else//����������Ϊ0
							{
								check_control_flag_main = 0;
							}
							if(ret == 1)//�Ѿ�������˻�û�п�����
							{
								fseek(fp, 1, SEEK_SET);
								unsigned char writebuf[2];
								writebuf[0] = 1;//��������lampȫ�������
								fwrite(writebuf, 1, 1, fp);
								check_control_flag_main = 0;
							}
						}
					}
					fclose(fp);
				}
			}
		}
		if(scene_info_flag)//�龰ģʽ LcpSyncDefaultSceneInfo������scene_info_flag=1
		{
			pthread_mutex_lock(&scene_file_mutex); 
			FILE * fp = fopen(FILE_SCENE, "rb+");// ��д��һ���������ļ���ֻ�����д���ݡ�
			if(fp == NULL)
			{
				printf("file /mnt/scene.info is not exist! \n");
				if(access(FILE_SCENE, F_OK) == 0)return;//return����ӵ�
				else
				{
					fp = fopen(FILE_SCENE, "wb+");//��д�򿪻���һ���������ļ����������д
					if(fp == NULL)
					{
						printf("file /mnt/scene.info is creat error! \n");
					}
					else
					{
						char str_buf[27] = {0};
						str_buf[0] = Finish;//ͬ�����
						str_buf[1] = 0;//�ƾ߸���Ϊ0
						fwrite(str_buf, 1, 27, fp);//����Ϣд��FILE_SCENE�ļ���
						fclose(fp);
					}
				}
			}
			else//���ļ��򿪳ɹ�
			{
				char read_temp[27], scene_head_parm[27];
				fread(scene_head_parm, 1, 27, fp);//��ȡ�龰�ļ����ļ�ͷ��Ϣ
				int i, ret = 1, ret_1 = 1;
				static int status_scene = 0;//�龰״̬��־λ
				if(scene_head_parm[0] == Finish)//scene_head_parm[0]�����Ƿ�ͬ�����
				{
					scene_info_flag = 0;
					status_scene = 1;//�龰״̬��־λ��Ϊ1
				}
				else//δͬ�����
				{
					for(i = 0; i < scene_head_parm[1]; i++)//��ǰ�洢�ĵƾ߸���
					{
						fread(read_temp, 1, 27, fp);//�ٶ�ȡ��27���ֽ�
						if(read_temp[0] != Finish)//û��ͬ�����
						{
							char mac_scene[8];
							ret = 0;
							if(status_scene == 0)
							{
								read_temp[1] = 0;//���ط�������0
								fseek(fp, -27, SEEK_CUR);
								fwrite(read_temp, 1, 27, fp);
								continue;
							}
							if(read_temp[1] != send_scene_info_count)continue;
							memcpy((char *)&mac_scene[4], (char *)&read_temp[2], 4);//3~6��MAC��ַ
							if (isExistNode(mac_scene) == FALSE)
							{
								struct scene_rsp temp;
								temp.msg_head = 0x7E;
								temp.msg_len = sizeof(struct scene_rsp);
								temp.msg_id = MSG_Scene_Sync;//�ƾ��龰ģʽͬ����Ϣ
								memcpy(temp.lamp_id, (char *)&read_temp[2], 4);//��MAC��ַ����lamp_id��
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

					if(ret)//ͬ�����
					{
						fseek(fp, 0, SEEK_SET);//��λ���ļ���ͷ
						unsigned char writebuf[2];
						writebuf[0] = 1;
						fwrite(writebuf, 1, 1, fp);//ֻ�е�һλΪ1,�������
						scene_info_flag = 0;
					}
					if(ret_1)//����Ƶ�MAC��ַ��ƥ����Ͼͼ���ִ��
					{
						send_scene_info_count++;
						if(send_scene_info_count == Send_Scene_Sync_Times)//ֹͣ����
						{	
							scene_info_flag = 0;
							send_scene_info_count = 0;//����
						}
					}
					if(status_scene == 0)status_scene = 1;//û��ͬ���ɹ�
				}
				fclose(fp);
			}
			pthread_mutex_unlock(&scene_file_mutex); 
		}
		usleep(1000000);
	}
	pthread_exit(0);
}


