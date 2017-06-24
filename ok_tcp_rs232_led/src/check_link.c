#include "tcp_rs232.h"
#include "nodearray.h"
#include "fun_msgpack.h"
#include "zigbeemsg.h"
#include <unistd.h>


int sockfd_server = -1;
unsigned char zigbee_tcp_connect = 0;

char get_mac (char *buf, int size)
{
	FILE *fp = NULL;

	if ((fp = popen("ifconfig eth0 | grep 'HWaddr' | awk -F ' ' '{printf $5}'", "r")) == NULL) 
	{
	#ifdef DEBUG
		perror("popen");
	#endif

		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "ERROR   %s", strerror(errno));
		log_to_file(__func__, "ifconfig eth0 | grep 'HWaddr' | awk -F ' ' '{printf $5}'", log_buf);

		sprintf(buf, "00:00:00:00:00:00");
		return -1;
	}
	else 
	{
		fgets(buf, size + 1, fp);
		if( !strlen(buf) )
		{
			sprintf(buf, "00:00:00:00:00:00");			
		}
		pclose(fp);
	}
	return 0;
}


/*************************************************
Function: check_link
Description:
Calls: tcp_send()
Called By: tcp_recv()
Input: 
Output: none
Return: none
Others: none
*************************************************/
void *check_link(void *arg)
{
	int i;
	int ret;
	pzbnode ptempnode = NULL;

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_zbomcparam.serverport);
	addr.sin_addr.s_addr = inet_addr(g_zbomcparam.serverip);

	zbnode tempnode;
	char mac_buf[24] = {0};
	
	get_mac(mac_buf, 18);//��ȡ���ص�mac��ַ
	
	fun_msgpack_init();//MSGPACK init
	while(1)
	{
		if(sockfd_server == -1 && zigbee_tcp_connect == 2)//��tcp�Ͽ����ӣ���������
		{
			zigbee_tcp_connect = 0;
			for(i = 0; i < MAXZBNODECOUNT; i++)
			{
				ptempnode = getNODEbyIndex(i);//��ýڵ��־����
				if(ptempnode == NULL)continue;
				pthread_mutex_lock(&nodearraymutex);
				ptempnode->loginflag = 1;
				pthread_mutex_unlock(&nodearraymutex); 
			}
		}

		if(tcp_num_rigester >= 4)//�����������4�ξ͹ر�tcp
		{
			printf("(tcp_num_rigester>=4,tcp is close\n");
			tcp_num_rigester = 0;
			close(sockfd_server);
			sockfd_server = -1;
			usleep(500000);
		}
		//tcp_recv�߳�ִ��tcp_recive_unpack��tcp_recive_cmd�᷵��zigbee_tcp_connect��ֵ���ж��Ƿ�����
		if(zigbee_tcp_connect == 2)//���ӳɹ�,���ƾ����Ӽ��ƾߵ�¼��msg_LcpLinkInitRsp������ֵ��
		{
			static long KeepAlivetime = 0;
			unsigned char connect_lamp_num = 0, connect_pad_num = 0;
			long currenttime = getcurrenttime();
			if((currenttime - KeepAlivetime) >= 4)//����������ÿ��4��
			{
				KeepAlivetime = currenttime;
				tcp_num_rigester++;//���������Ļ�һֱΪ0
				msgpack_sbuffer_clear(g_pk_buffer);
				msgpack_pack_array(g_pk, 1);
				msgpack_pack_uint32(g_pk, LCP_LINK_KEEPALIVE);
                
				tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
			}
			for(i = 0; i < MAXZBNODECOUNT; i++)//����ж����豸��������
			{
				ptempnode = getNODEbyIndex(i);
				if(ptempnode == NULL)continue;
				//if (ptempnode->zbnodelive_val >= ZB_OFF_LINE)continue;

				if(ptempnode->lamp_type < TYPE_PAD)//rs232_recv->uart_recv_unpack_7E->RSP_ID_NET_HeartBeat->lamp_type
				{
					if(ptempnode->loginflag == 1)connect_lamp_num++;//�����ϵĵƵĸ����ۼ�
					else if(ptempnode->loginflag == 2 && (currenttime - (ptempnode->logintime) > 5))connect_lamp_num++;
					if(connect_lamp_num == Send_Lamp_Login_Num)break;
				}
				else
				{
					if(ptempnode->loginflag == 1)connect_pad_num++;//���������� ���ͺ��⴫����
					else if(ptempnode->loginflag == 2 && (currenttime - (ptempnode->logintime) > 5))connect_pad_num++;
				}
			}
			if(connect_lamp_num != 0)//�ƾ�׼����¼
			{
				msgpack_sbuffer_clear(g_pk_buffer);
				msgpack_pack_array(g_pk, 2);
				msgpack_pack_uint32(g_pk, LCP_LAMP_LOGIN);//�ƾߵ�¼
				msgpack_pack_array(g_pk, connect_lamp_num);
				char mac_tempbuff[20] = {0};
				int join_send_num = 0;//���ڼ�¼�Ƶ�¼�ɹ��ĸ���
				for(i = 0; i < MAXZBNODECOUNT; i++)//����ж����豸��������
				{
					ptempnode = getNODEbyIndex(i);
					if(ptempnode == NULL)continue;
					//if (ptempnode->zbnodelive_val >= ZB_OFF_LINE)continue;
					if(ptempnode->loginflag == 1);
					else if(ptempnode->loginflag == 2 && (currenttime - (ptempnode->logintime) > 5));
					else continue;
					if(ptempnode->lamp_type >= TYPE_PAD)continue;
					msgpack_pack_array(g_pk, 4);

                    #ifdef TCP_SEND_DEBUG										
                    printf("lamp[%d] ready login to server, mac:", i++);					
                    printhex((char *)&ptempnode->macaddr, 8);					
                    #endif
                    
					int m = 0;
					//mac��ַ����ֻ��Ҫ����6���ֽھͿ�����,ǰ�������ǹ̶�����0x000D
					for(m = 0; m < 6; m++)sprintf(&mac_tempbuff[3 * m], "%02X:", (unsigned int)ptempnode->macaddr[m + 2]);
					msgpack_pack_str(g_pk, MAC_LEN); 
					msgpack_pack_str_body(g_pk, mac_tempbuff, MAC_LEN);
					msgpack_pack_uint32(g_pk, ptempnode->lamp_type);
					msgpack_pack_uint32(g_pk, ptempnode->power);
					msgpack_pack_uint32(g_pk, 0);

					pthread_mutex_lock(&nodearraymutex);
					ptempnode->loginflag = 2;//���ӳɹ���loginflag��־λ��2����ȫ�ֱ���g_zbnodearray.node[i].loginflag=2
					ptempnode->logintime = currenttime;//�����ӳɹ�ʱ�ĵ�ǰʱ�䱣�浽logintime
					pthread_mutex_unlock(&nodearraymutex); 
					join_send_num++;
					if(join_send_num == connect_lamp_num)break;//�����¼���豸�������������ϵĵ���
				}
				tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
				connect_lamp_num = 0;
				
			}
			if(connect_pad_num != 0)//�������
			{
				msgpack_sbuffer_clear(g_pk_buffer);
				msgpack_pack_array(g_pk, 2);
				msgpack_pack_uint32(g_pk, LCP_CTRL_PANEL_LOGIN);
				msgpack_pack_array(g_pk, connect_pad_num);
				char mac_tempbuff[20] = {0};
				for(i = 0; i < MAXZBNODECOUNT; i++)//����ж����豸��������
				{
					ptempnode = getNODEbyIndex(i);
					if(ptempnode == NULL)continue;
					if(ptempnode->lamp_type < TYPE_PAD)continue;
					//if (ptempnode->zbnodelive_val >= ZB_OFF_LINE)continue;
					if(ptempnode->loginflag == 1);
					else if(ptempnode->loginflag == 2 && (currenttime - (ptempnode->logintime) > 5));
					else continue;
					msgpack_pack_array(g_pk, 2);
                    
                    #ifdef TCP_SEND_DEBUG									
                    printf("pad[%d] ready login to server, mac:", i++);					
                    printhex((char *)&ptempnode->macaddr, 8);					
                    #endif
					
					int m = 0;		
					//mac��ַ����ֻ��Ҫ����6���ֽھͿ�����
					for(m = 0; m < 6; m++)sprintf(&mac_tempbuff[3 * m], "%02X:", (unsigned int)ptempnode->macaddr[m + 2]);
					msgpack_pack_str(g_pk, MAC_LEN); 
					msgpack_pack_str_body(g_pk, mac_tempbuff, MAC_LEN);//����MAC��ַ
					msgpack_pack_uint32(g_pk, 0);//��������

					pthread_mutex_lock(&nodearraymutex);
					ptempnode->loginflag = 2;//���ӳɹ���loginflag��־λ��2
					ptempnode->logintime = currenttime;//�����ӳɹ�ʱ�ĵ�ǰʱ�䱣�浽logintime
					pthread_mutex_unlock(&nodearraymutex); 
				}
				tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
				connect_pad_num = 0;
				
			}
		}
		else if(zigbee_tcp_connect == 0)//δ���ӷ����������ؿ�ʼ���ӷ�����
		{
			sockfd_server = socket(AF_INET, SOCK_STREAM, 0);//�����������׽���
			if(sockfd_server <= 0)
			{
				printf("socket error, socket=%d\r\n", sockfd_server);
				usleep(500000);
				continue;
			}
							
			struct timeval timeout_recv = {2,0};
			//������������һЩԭ���շ����ܰ�ʱ���У��������ý���ʱ��Ϊ2s
			setsockopt(sockfd_server, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_recv, sizeof(timeout_recv));
				
			if(connect(sockfd_server, (struct sockaddr *)&addr, sizeof(addr)) != 0) //connect to server fail
			{
				printf("connect to server fail!!! sockfd=%d\n", sockfd_server);
				close(sockfd_server);
				sockfd_server = -1;
				usleep(500000);
				continue;
			}
			else//�������ӷ������ɹ�����ʼ��¼������
			{  
				#ifdef DEBUG
				printf("connect to server success!!!\r\n");
			    #endif
				int i = 0, j = 0;
				char mac_tempbuf[20] = {0};
				for(i = 0; i < MAC_LEN; i++)//mac��ַת��00:00:00:00:00:00  to  000000000000
				{
					if(mac_buf[i] != ':')
					{
						mac_tempbuf[j] = mac_buf[i];
						j++;
					}
				}
				msgpack_sbuffer_clear(g_pk_buffer);
				msgpack_pack_array(g_pk, 4);
				msgpack_pack_uint32(g_pk, LCP_LINK_INIT);
				msgpack_pack_str(g_pk, 12); 
				msgpack_pack_str_body(g_pk, mac_tempbuf, 12);
				msgpack_pack_str(g_pk, 12); 
				msgpack_pack_str_body(g_pk, mac_tempbuf, 12);
				msgpack_pack_uint32(g_pk, 1);
				zigbee_tcp_connect = 1;
                
				tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);//�����ݷ��͵��������ļ���
			}
		}
		else if(zigbee_tcp_connect == 1)//20����δ�յ���½��Ӧ����Ͽ�socket����������
		{
			static int timenum = 0;
			timenum++;
			if(timenum == 20)
			{
				timenum = 0;
				zigbee_tcp_connect = 0;
				close(sockfd_server);
				sockfd_server = -1;
				printf("send login outtime\n");
			}	
		}
		usleep(1000000);
	}
	memset(log_buf, '\0', sizeof(log_buf));
	sprintf(log_buf, "pthread  check_link() exit ERROR :errno=%d -> %s", errno, strerror(errno));
	log_to_file(__func__, log_buf, NULL);
	pthread_exit(0);
}

