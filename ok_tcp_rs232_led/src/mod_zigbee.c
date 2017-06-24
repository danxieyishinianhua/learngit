#include "tcp_rs232.h"
#include "send_list.h"

extern void ReadzigbeeConfig(void);

//MAC 地址的高 4 字节固定。因此在对模块进行寻址时，只使用低 4 字节的 MAC 地址即可
void *zb_live_check(void *arg)
{
	long lasttraveltime = getcurrenttime();
	int interval;
  	while(1)
	{
		usleep(20000000);//20s

		//if(IsZigbeeDebug() > 0)continue;
		long currenttime = getcurrenttime();
		zigbee_node_heratbeat_outtime(currenttime);//检查心跳是否超时
		
		/*pzbnode pnode = gettestneighborNode(currenttime);
		if(pnode != NULL)
		{
			char tempsendbuf[10];
			uint8 srchexmac[4];
			uint8 dsthexmac[4];
			memset(srchexmac, 0, sizeof(srchexmac));
			tempsendbuf[0] = NEIGHBORTABLE%256;//read index 0x03E9
			tempsendbuf[1] = NEIGHBORTABLE/256;//read index
			tempsendbuf[2] = ALLPARAM;//read SUB index
			tempsendbuf[3] = 0;//read OPT
			int len = 4;
			char sendbuf[128];
			dsthexmac[0] = pnode->macaddr[7];
			dsthexmac[1] = pnode->macaddr[6];
			dsthexmac[2] = pnode->macaddr[5];
			dsthexmac[3] = pnode->macaddr[4];
			//memcpy(dsthexmac, node->zb_data.macaddr, len); //mac 地址顺序是颠倒的
			unsigned short cmd = 0x3800|READFLAG;
			int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
			if(datalen > 0)
			{
				printf("send test pack mac=%02X%02X%02X%02X\r\n", dsthexmac[3], dsthexmac[2],dsthexmac[1],dsthexmac[0]);
				printhex(sendbuf, datalen);

				msg_write_to_list(sendbuf, datalen);
			}
		}*/
		interval = currenttime - lasttraveltime;
		if(interval > 60){
			lasttraveltime = currenttime;
			//HandlezbnodeState();//灯具自己判断是否在线
			SaveZigbeeNode();
		}
  	}
}

void *zigbeenetworking(void *arg)
{
	int i;
	struct ZLL_CONFIG config;
	config.msg_head = 0x7e;
	config.msg_len  = 6;
	config.msg_id   = ID_NET_Config;
	config.msg_end  = 0x5e;
	

#define MAXCONFIGCOUNT 16
struct zpconfig zpconfigarray[MAXCONFIGCOUNT] =
	{ {"686E518C", 0x0B}, {"686E518D", 0x0B}, {"686E518E", 0x0B}, {"686E518F", 0x0B}, 
	  {"686E518C", 0x0F}, {"686E518D", 0x0F}, {"686E518E", 0x0F}, {"686E518F", 0x0F}, 
	  {"686E518C", 0x14}, {"686E518D", 0x14}, {"686E518E", 0x14}, {"686E518F", 0x14}, 
	  {"686E518C", 0x19}, {"686E518D", 0x19}, {"686E518E", 0x19}, {"686E518F", 0x19}
	};
	
   	while(1)
	{
		ReadzigbeeConfig();
	
		if(g_zbnetworkparam.networkenable != 0)
		{
			if(config.on_off == 0)
			{
				config.on_off = 1;
			}
			else
			{
				config.on_off = 0;
			}
			
			for(i = 0; i < MAXCONFIGCOUNT; i++)//16
			{
				if ((memcmp(g_zbnetworkparam.panid, zpconfigarray[i].panid, 8) == 0) && (g_zbnetworkparam.channel == zpconfigarray[i].channel) )
				{
					config.net_id = i;
					break;
				}
			}
	        msg_write_to_list((char*)&config, config.msg_len);
		}
		usleep(3000000);	
		
	}
	
	memset(log_buf, '\0', sizeof(log_buf));
	sprintf(log_buf, "pthread zigbeenetworking() exit ERROR :errno=%d -> %s", errno, strerror(errno));
	log_to_file(__func__, log_buf, NULL);
	pthread_exit(0);
}
