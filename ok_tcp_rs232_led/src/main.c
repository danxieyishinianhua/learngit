#include "tcp_rs232.h"
#include "nodearray.h"
#include "serial.h"
#include "send_list.h"

ZBNETWORKPARAM g_zbnetworkparam;
ZBOMCPARAM g_zbomcparam;
extern void *tcpserverhandle(void *arg);
void trim(char *buffer, int buflen)//����Ƿ��лس���������Ǿͽ���
{
	int i;
	for(i = 0; i < buflen; i++)
	{
		if((buffer[i] == 0x0d) || (buffer[i] == 0x0a))//0x0d��ʾ�س���,0x0a��ʾ���з�
		{
			buffer[i] = 0;
			break;
		}
	}
}
void fileDev()
{

 int fd;
 fd = open(LOG_FILE_PATH, O_CREAT | O_RDWR | O_EXCL, 0666); //O_EXCL ���ͬʱָ����O_CREAT,�����ļ��Ѵ���,������ء�
 close(fd);
}

void initdefaultconfig()
{
	g_zbnetworkparam.cmdtrycount = CMDTRYCOUNT;//4��
	g_zbnetworkparam.cmdtimeout = CMDTIMEOUT;//1s
	g_zbnetworkparam.cmdsendinterval = CMDINTERVAL;//100
	g_zbnetworkparam.neighborinterval = NEIGHBORINTERVAL;//300

	sprintf(g_zbnetworkparam.panid, "686E518C");
	g_zbnetworkparam.channel = 0x19;	
}
void initwificonfig()//����wifi��
{
	FILE *fp = NULL;
	char buf[128] = {0};
	char mac_buf[24] = {0};
	char tempbuf[18] = {0};
	//popen() ���� �� �����ܵ� �� ��ʽ����һ�� ����, ������ shell. ��Ϊ �ܵ��Ǳ�����ɵ���ġ�
	if ((fp = popen("uci -q get wireless.@wifi-device[0].disabled", "r")) == NULL)//uciϵͳĬ���﷨
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get wireless.@wifi-device[0].disabled", log_buf);
	}
	else
	{
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( strlen(buf) > 0 )
			{
				buf[strlen(buf)] = '\0';
				
				if(memcmp(buf, "1", strlen(buf)) == 0)
				{
					system("uci set wireless.@wifi-device[0].disabled=0");//������
					system("uci commit wireless"); //д������
				}
			}	
		}
		fclose(fp);
	}
    
	if ((fp = popen("uci -q get wireless.@wifi-iface[0].ssid", "r")) == NULL)//uciϵͳĬ���﷨
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get wireless.@wifi-iface[0].ssid", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( strlen(buf) > 4 )
			{
				buf[strlen(buf)] = '\0';	
				get_mac(mac_buf, 18);
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
                sprintf(tempbuf, "MODGW_%s", &mac_tempbuf[MAC_offset]);
				if(memcmp(tempbuf, buf, strlen(tempbuf)) != 0)
				{
					memset(buf, '\0', sizeof(buf));
    				sprintf(buf, "uci set wireless.@wifi-iface[0].ssid=MODGW_%s", &mac_tempbuf[MAC_offset]);
    				system(buf);
    				system("uci commit wireless"); //д������
    				printf("reset wireless success!!!\r\n");
				}	
			}
		}
		pclose(fp);
	}
}

void ReadzigbeeConfig(void)
{
	FILE *fp = NULL;
	char buf[16];
	//popen() ���� �� �����ܵ� �� ��ʽ����һ�� ����, ������ shell. ��Ϊ �ܵ��Ǳ�����ɵ���ġ�
	if ((fp = popen("uci -q get zigbee.@param[0].cmdtrycount", "r")) == NULL)//uciϵͳĬ���﷨
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].cmdtrycount", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) > 0 )
			{
				g_zbnetworkparam.cmdtrycount = atoi(buf);
			}
		}
		pclose(fp);
	}

	if ((fp = popen("uci -q get zigbee.@param[0].cmdtimeout", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].cmdtimeout", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) > 0 )
			{
				g_zbnetworkparam.cmdtimeout = atoi(buf);
			}
		}
		pclose(fp);
	}

	if ((fp = popen("uci -q get zigbee.@param[0].cmdsendinterval", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].cmdsendinterval", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) > 0 )
			{
				g_zbnetworkparam.cmdsendinterval = atoi(buf) * 1000;
			}
		}
		pclose(fp);
	}

	if ((fp = popen("uci -q get zigbee.@param[0].neighborinterval", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].neighborinterval", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) > 0 )
			{
				g_zbnetworkparam.neighborinterval = atoi(buf);
			}
		}
		pclose(fp);
	}

	if ((fp = popen("uci -q get zigbee.@param[0].PanID", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].PanID", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( strlen(buf) > 7 )
			{
				buf[strlen(buf)] = '\0';
				memcpy(g_zbnetworkparam.panid, buf, sizeof(buf));
			}
		}
		pclose(fp);
	}

	if ((fp = popen("uci -q get zigbee.@param[0].Channel", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].Channel", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) > 0 )
			{
				g_zbnetworkparam.channel = atoi(buf);
			}
		}
		pclose(fp);
	}
	
	if ((fp = popen("uci -q get zigbee.@param[0].networkenable", "r")) == NULL)
	{

		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].networkenable", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) != 0 )
			{
				g_zbnetworkparam.networkenable = 1;
			}
			else
			{
				g_zbnetworkparam.networkenable = 0;
			}
		}
		pclose(fp);
	}
}
void ReadOMCConfig(void)
{
	FILE *fp = NULL;
	char buf[16];
	if ((fp = popen("uci -q get zigbee.@param[0].ServerIP", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].ServerIP", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( strlen(buf) > 7 )
			{

				buf[strlen(buf)] = '\0';
				memcpy(g_zbomcparam.serverip, buf, sizeof(buf));
			}
		}
		pclose(fp);
	}

	if ((fp = popen("uci -q get zigbee.@param[0].ServerPort", "r")) == NULL)
	{
	#ifdef DEBUG
		perror("uci");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "   ERROR   %s", strerror(errno));
		log_to_file(__func__, "uci -q get zigbee.@param[0].ServerPort", log_buf);
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp) != NULL)
		{
			trim(buf, strlen(buf));
			if( atoi(buf) > 0 )
			{
				g_zbomcparam.serverport = atoi(buf);
			}
		}
		pclose(fp);
	}	
}
//panid:zigbee Ĭ��484F4D41 mod:686E518C
int main (int argc, char *argv[] )
{
	signal(SIGCHLD, SIG_IGN);//SIGCHLD����һ��������ֹ����ֹͣʱ����SIGCHLD�źŷ��͸��丸����
	signal(SIGPIPE, SIG_IGN);//������signal(SIGPIPE, SIG_IGN), ��������  SIGPIPE �ź�ʱ�Ͳ�����ֹ����ֱ�Ӱ�����źź��Ե���
	signal(SIGTRAP, SIG_IGN);//SIGTRAP ����/�ϵ��ж�

	fileDev(); 
	initdefaultconfig();
    ReadzigbeeConfig();
	ReadOMCConfig();
    
	if (argc > 2)
	{
		strcpy( g_zbomcparam.serverip, argv[1]);
		g_zbomcparam.serverport = atoi(argv[2]);
	}

	char g_version []= "V2.1.20170525";
    
		printf("server initting version=%s %s %s\r\n	serverip:%s	port:%d\r\n", g_version, __TIME__,__DATE__, g_zbomcparam.serverip, g_zbomcparam.serverport);
		printf("panid:%schannel:%d\r\n", g_zbnetworkparam.panid, g_zbnetworkparam.channel);
		printf("cmdtrycount:%d,cmdtimeout:%d,cmdsendinterval:%d,neighborinterval:%d \r\n", 	g_zbnetworkparam.cmdtrycount,	g_zbnetworkparam.cmdtimeout,
			g_zbnetworkparam.cmdsendinterval,	g_zbnetworkparam.neighborinterval);
    
	rs232_fd = uart_setup(RS232_DEVICE, RS232_BAUDRATE);
	if (rs232_fd < 0)
	{
	    #ifdef DEBUG
		printf("%s open error!\n", RS232_DEVICE);
	    #endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "%s open error!\n", RS232_DEVICE);
		log_to_file(__func__, log_buf, NULL);//__func__����main
		return 0;
	}
	/* rs232_send_list initialization */
	memset((char *)&rs232_send_list, 0, sizeof(struct uart_send_buf)*(SEND_BUFF_SIZE + 1));
	/* mutex initialization */
	pthread_mutex_init(&mutex_list, NULL);
	/* condition variables initialization */
	pthread_cond_init(&cond_list, NULL);//����pthread_cond_wait����ʹ�߳�������һ������������
	
	//struct tree
	zbnodearrayInit();

	InitZigbee(&g_zbnetworkparam);

	initwificonfig();
	//check tree data ->check zigbee live
	pthread_create(&pth_zb_live_check, 0, zb_live_check , NULL);

	pthread_create(&pth_link, 0, check_link, NULL);

	/* SOCKET communication with the server */
	pthread_create(&pth_tcp_recv, 0, tcp_recv, NULL);
	/* UART read data */
	pthread_create(&pth_rs232_read, 0, rs232_recv, NULL);
	/* UART write data */
	pthread_create(&pth_rs232_write, 0, rs232_write_list, NULL);

	pthread_create(&pth_tcpserver, 0, tcpserverhandle, NULL);
    
	pthread_create(&pth_zigbeenetworking, 0, zigbeenetworking, NULL);//�Է��͹㲥����ʽ������ͬһ����ĵ�������

	pthread_create(&pth_rs232_resend, 0, rs232_resend, NULL);

	/* Wait for the end of the thread */
	pthread_join(pth_rs232_write, NULL);
	pthread_join(pth_rs232_read, NULL);
	pthread_join(pth_link, NULL);
	pthread_join(pth_tcp_recv, NULL);
	pthread_join(pth_zb_live_check, NULL);
	pthread_join(pth_tcpserver, NULL);
	pthread_join(pth_zigbeenetworking, NULL);
	pthread_join(pth_rs232_resend, NULL);
	
	pthread_cond_destroy(&cond_list); 
	pthread_mutex_destroy(&mutex_list);
	zbnodearrayDestory();
	
	close(rs232_fd);
	return 0;
}

