#include <sys/socket.h>
#include "nodearray.h"
#include "tcp_rs232.h"
#include "fun_msgpack.h"
#include "send_list.h"


zbnodearray g_zbnodearray;
int g_initflag = 1;
int g_testindex = 0;
pthread_mutex_t nodearraymutex = PTHREAD_MUTEX_INITIALIZER;


int  addNewzbNode(pzbnode pnode )//添加新的zb节点
{
	int i;
	printf("*****addNewzbNode!!!*****\r\n");
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if(g_zbnodearray.userflag[i] == 0)
		{
			memcpy(&(g_zbnodearray.node[i]), pnode, sizeof(zbnode));
			
			g_zbnodearray.userflag[i] = 1;//说明有设备连接上了，添加新的节点
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return 0;
}

int  delzbNode(int index)
{
	if((index < 0) || (index >= MAXZBNODECOUNT))
	{
		return 0;
	}
	pthread_mutex_lock(&nodearraymutex); 

	if(g_zbnodearray.node[index].socket > 0)
	{
		close(g_zbnodearray.node[index].socket);
	}
	memset(&(g_zbnodearray.node[index]), 0, sizeof(zbnode));
	g_zbnodearray.userflag[index] = 0;//删除节点
	pthread_mutex_unlock(&nodearraymutex); 
	return 0;
}
pzbnode getNODEbyIndex(int index)
{
	pzbnode pnode = NULL;
	if((index < 0) || (index >= MAXZBNODECOUNT))
	{
		return NULL;
	}

	pthread_mutex_lock(&nodearraymutex); 

	if(g_zbnodearray.userflag[index] > 0)
	{
		pnode = &(g_zbnodearray.node[index]);
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return pnode;
}
pzbnode getNODEbysocket(int socket)//该函数未使用
{
	int i;
	pzbnode pnode = NULL;
	if(socket < 0) 
	{
		return NULL;
	}

	pthread_mutex_lock(&nodearraymutex); 
	for (i = 0; i < MAXZBNODECOUNT; i++)
	{
		if ((g_zbnodearray.userflag[i] == 1) &&(g_zbnodearray.node[i].socket == socket))
		{
			pnode = &(g_zbnodearray.node[i]);
		}
	}

	pthread_mutex_unlock(&nodearraymutex); 
	return pnode;
}

void CloseSocketbyindex(int index)//该函数未使用
{
	if((index < 0) || (index >= MAXZBNODECOUNT))
	{
		return;
	}

	pthread_mutex_lock(&nodearraymutex); 

	if((g_zbnodearray.userflag[index] > 0) && (g_zbnodearray.node[index].socket > 0))
	{
		close(g_zbnodearray.node[index].socket);
		g_zbnodearray.node[index].socket = 0;
		g_zbnodearray.node[index].loginflag = 0;
		g_zbnodearray.node[index].heartbeatfailcount = 0;
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ;
}
void zbnodearrayInit()
{
	pthread_mutex_lock(&nodearraymutex); 

	memset(&g_zbnodearray, 0 , sizeof(g_zbnodearray));
	
	pthread_mutex_unlock(&nodearraymutex); 
}
void zbnodearrayDestory()//节点销毁函数
{
	int i;
	printf("zbnodearrayDestory \r\n ");
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if(g_zbnodearray.node[i].socket > 0)
		{
			close(g_zbnodearray.node[i].socket);
		}
	}
	memset(&g_zbnodearray, 0 , sizeof(g_zbnodearray));
	
	pthread_mutex_unlock(&nodearraymutex); 
}


pzbnode gettestneighborNode(long currentime)//该函数未使用
{
	int i;
	long interval = 0;//currentime - node.testtime;
	pzbnode ptempnode = NULL;
	pthread_mutex_lock(&nodearraymutex); 
	
	for(i = g_testindex; i < MAXZBNODECOUNT; i++)//100
	{
		if(g_zbnodearray.userflag[i] != 0)
		{
			pzbnode node = &g_zbnodearray.node[i];
			interval = currentime - g_zbnodearray.node[i].testtime;
			if((interval >= g_zbnetworkparam.neighborinterval) && (node->zbnodelive_val > 0x10))//>0x10下线了是厂家定义的
			{ 
				node->testtime = currentime;
				ptempnode = &g_zbnodearray.node[i];
				node->testalivecount++;
		    	break;
			}
			if (node->zbnodelive_val < 0x10)
			{
				node->testalivecount = 0;
			}
		}
	}
	g_testindex = (i + 1);
	if (i >= MAXZBNODECOUNT)
	{
		g_testindex = 0;
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ptempnode;
}
void printnode(pzbnode node)
{

	printf("__func=%s, prt=%x macaddr=0x%02X%02X%02X%02X%02X%02X%02X%02X, zbnodelive_val=%d,nwkaddr=(0x%x,0x%x) socketid=%d heartbeatfailcount=%d, heartbeatinterval=%d heartbeattime=%ld loginflag=%d socketstarttime=%ld\n", __func__, (unsigned int)node,
	node->macaddr[0], node->macaddr[1], node->macaddr[2], node->macaddr[3], node->macaddr[4],
	node->macaddr[5], node->macaddr[6], node->macaddr[7], node->zbnodelive_val, node->nwkaddr[0], node->nwkaddr[1], node->socket,
	node->heartbeatfailcount, node->heartbeatinterval, node->heartbeattime, node->loginflag, node->socketstarttime);
    int i;
	for(i = 0; i < MAXNEIGHBORCOUNT; i++)
	{
		printf("neighbor[%d]=0x%02X%02X%02X%02X rssi=%d, fail=%d testtime=%ld\n", i, 
		node->neighbor[i].uiNode_Addr[0], node->neighbor[i].uiNode_Addr[1],
		node->neighbor[i].uiNode_Addr[2], node->neighbor[i].uiNode_Addr[3],
		node->neighbor[i].cRSSI, node->neighbor[i].ucFailure_Or_Age, node->testtime);
	}
}
void nodearray_print()//该函数未使用
{
	int i;
	printf("nodearray_print\r\n");
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if(g_zbnodearray.userflag[i] != 0) 
		{
			printnode(&g_zbnodearray.node[i]);
		}
	}

	pthread_mutex_unlock(&nodearraymutex); 
	return;
}
void updatezbnodelive_val(char *mac,int var )
{
	int i;
    printf("updatezbnodelive_val is starting!!!\n");
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)//100
	{
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, mac, 8) == 0))
		{
			g_zbnodearray.node[i].zbnodelive_val = var;
			if(g_zbnodearray.node[i].loginflag != 3)g_zbnodearray.node[i].loginflag = 1;
			g_zbnodearray.node[i].heartbeattime = getcurrenttime();
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}
void setzbnodecmdbuf(char *mac,char *cmdbuf, int buflen )//该函数未使用
{
	int i;
	if(IsZigbeeDebug() > 0)
	{
		return;
	}
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, mac, 8) == 0))
		{
			if(buflen < MAXBUFLEN)
			{
				memcpy(g_zbnodearray.node[i].cmdbuf, cmdbuf, buflen);
				g_zbnodearray.node[i].cmdbuflen = buflen;
				g_zbnodearray.node[i].cmdtrycount = g_zbnetworkparam.cmdtrycount;
				g_zbnodearray.node[i].commandtime = getcurrenttime() - 100;
				printf("setzbnodecmdbuf cmdlen=%d\r\n", buflen);
				printhex(cmdbuf, buflen);
			}
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}


int re_send_light_cmd(void)//检查灯具是否需要重发命令
{
	char cmd_buf[MAX_UART_SEND_LEN] = {0}, scene_buf[MAX_UART_SEND_LEN] = {0}; 
	int i, num = 0, scene_num = 0, ret = 0;

	pthread_mutex_lock(&nodearraymutex);
	
	cmd_buf[0] = HEADFLAG;
	cmd_buf[2] = MSG_SET_LAMPS;
	scene_buf[0] = HEADFLAG;
	scene_buf[2] = ID_DATA_ReSet_Scene;
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			if (g_zbnodearray.node[i].resend_cmd_flag == 1)//调光需要重发
			{
				memcpy(&cmd_buf[num * 8 + 4], &g_zbnodearray.node[i].macaddr[4], 4);
				if(g_zbnodearray.node[i].lamp_type == 2)//RGB
				{
					cmd_buf[num * 8 + 8]  = g_zbnodearray.node[i].lightness;
					cmd_buf[num * 8 + 9]  = g_zbnodearray.node[i].red;
					cmd_buf[num * 8 + 10] = g_zbnodearray.node[i].green;
					cmd_buf[num * 8 + 11] = g_zbnodearray.node[i].blue;
				}
				else cmd_buf[num * 8 + 9] = g_zbnodearray.node[i].colortemp, cmd_buf[num * 8 + 8] = g_zbnodearray.node[i].bringhtness;
				num++;
				if(num == ((MAX_UART_SEND_LEN - 5) / 8))
				{
					cmd_buf[1] = num * 8 + 5;
					cmd_buf[3] = num;
					cmd_buf[num * 8 + 4] = ENDFLAG;

					ret += num;
					num = 0;
					msg_write_to_list((char *)&cmd_buf, cmd_buf[1]);
				}
			}
			else if(g_zbnodearray.node[i].resend_cmd_flag == 2)//情景需要重发
			{
				memcpy(&scene_buf[scene_num * 5 + 4], &g_zbnodearray.node[i].macaddr[4], 4);
				scene_buf[scene_num * 5 + 8] = g_zbnodearray.node[i].scene_key;
				scene_num++;
				if(scene_num == ((MAX_UART_SEND_LEN - 5) / 5))
				{
					scene_buf[1] = scene_num * 5 + 5;
					scene_buf[3] = scene_num;
					scene_buf[scene_num * 5 + 4] = ENDFLAG;

					ret += scene_num;
					scene_num = 0;
					msg_write_to_list((char *)&scene_buf, scene_buf[1]);
				}
			}
		}
	}
	
	pthread_mutex_unlock(&nodearraymutex);
	
	if(num != 0)
	{
		cmd_buf[1] = num * 8 + 5;
		cmd_buf[3] = num;
		cmd_buf[num * 8 + 4] = ENDFLAG;
		ret += num;
		msg_write_to_list((char *)&cmd_buf, cmd_buf[1]);
	}
	if(scene_num != 0)
	{
		scene_buf[1] = scene_num * 5 + 5;
		scene_buf[3] = scene_num;
		scene_buf[scene_num * 5 + 4] = ENDFLAG;
		ret += scene_num;
		msg_write_to_list((char *)&scene_buf, scene_buf[1]);
	}
	if(ret > 0)
	{
		#ifdef RESEND_ZB
		printf("re_send_light_cmd num:%d............\n", ret);
		#endif
	}
	else return ret;
}

int getzbnodecmdbuf(int index, char *cmdbuf)//该函数未使用
{
	int buflen = 0;
	long interval = 0;
	long currenttime = getcurrenttime();


	if((index < 0) || (index >= MAXZBNODECOUNT))
	{
		return 0;
	}
	pthread_mutex_lock(&nodearraymutex); 
	
	if (g_zbnodearray.userflag[index] != 0)
	{
		interval = currenttime - g_zbnodearray.node[index].commandtime;
		if (interval > g_zbnetworkparam.cmdtimeout)
		{			
			if(g_zbnodearray.node[index].cmdtrycount <= 0)
			{	
				pthread_mutex_unlock(&nodearraymutex); 
				return 0 ;
			}
			g_zbnodearray.node[index].cmdtrycount--;
			buflen = g_zbnodearray.node[index].cmdbuflen;
			if(buflen < MAXBUFLEN)
			{
				memcpy(cmdbuf,g_zbnodearray.node[index].cmdbuf, buflen);	
			}
			g_zbnodearray.node[index].commandtime = currenttime;
		}
	}
	
	pthread_mutex_unlock(&nodearraymutex); 
	return buflen;
}

void Deletenodecmdbuf(char *mac, int cmd)//该函数未使用
{
	int i;
	int buflen = 0;
	printf("Deletenodecmdbuf 0000 cmd=%02x\r\n", cmd);
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i< MAXZBNODECOUNT; i++)
	{
	//2A 2F 41 88 77 A4 B7 18 77 B0 00 00 00 00 66 9C 00 00 19 5D D7 0B 00 00 00 00 00 00 25 38 B6 F0 A0 0F 00 00 0C 00 66 00 0C 00 00 00 00 00 00 00 00 64 23 
		int msg_id = g_zbnodearray.node[i].cmdbuf[37+0]*256+g_zbnodearray.node[i].cmdbuf[37+1];
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, mac, 8) == 0) && (msg_id == cmd))
		{
			printf("Deletenodecmdbuf 1111\r\n");
			memset(g_zbnodearray.node[i].cmdbuf, 0, MAXBUFLEN);
			g_zbnodearray.node[i].cmdbuflen = 0;
			g_zbnodearray.node[i].cmdtrycount = 0;
			g_zbnodearray.node[i].zbnodelive_val= 0;
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ;
}
void updateTestneighborTime(char *mac,long time )//该函数未使用
{
	int i;

	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i< MAXZBNODECOUNT; i++)
	{
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, mac, 8) == 0))
		{
			g_zbnodearray.node[i].testtime = time;
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}
void updateHeartbeatTime(int index )//该函数未使用
{
	if((index < 0) || (index >= MAXZBNODECOUNT))
	{
		return;
	}

	pthread_mutex_lock(&nodearraymutex); 

	if (g_zbnodearray.userflag[index] != 0) 
	{
		g_zbnodearray.node[index].heartbeatfailcount++;
		g_zbnodearray.node[index].heartbeattime = getcurrenttime();
		
	}

	pthread_mutex_unlock(&nodearraymutex); 
	return;
}
void updateneighbor(char *mac,char *nodebuf, int nodebuflen)
{
	int i, j, count;
	char macbuf1[4];
	pthread_mutex_lock(&nodearraymutex);
	for(j = 0; j < (nodebuflen / 6); j++)
	{
		macbuf1[0] = nodebuf[3 + j * 6];
		macbuf1[1] = nodebuf[2 + j * 6];
		macbuf1[2] = nodebuf[1 + j * 6];
		macbuf1[3] = nodebuf[0 + j * 6];
		for(i = 0; i < MAXZBNODECOUNT; i++)
		{	
			if ((g_zbnodearray.userflag[i] != 0) && (memcmp((char *)&g_zbnodearray.node[i].macaddr[4], macbuf1, 4) == 0))
			{
				memcpy(&g_zbnodearray.node[i].neighbor[0].uiNode_Addr, (char *)&mac[4], 4);
				g_zbnodearray.node[i].neighbor[0].cRSSI = nodebuf[4 + j * 6];
				g_zbnodearray.node[i].neighbor[0].ucFailure_Or_Age = nodebuf[5 + j * 6];
				break;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}
void updatenodesocket(char *mac, int socket)//该函数未使用
{
	int i;
	printf("updatenodesocket\r\n");
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i< MAXZBNODECOUNT; i++)
	{
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, mac, 8) == 0))
		{
			g_zbnodearray.node[i].socket = socket;
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}

void updatenodeNode(pzbnode node)//该函数未使用
{
	int i;
	printf("updatenodeNode\r\n");
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i< MAXZBNODECOUNT; i++)
	{
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, node->macaddr, 8) == 0))
		{
			memcpy(&g_zbnodearray.node[i], node, sizeof(zbnode));
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}

bool isExistNode(char *mac)//节点是否存在
{
	int i;
	bool ret = FALSE;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			//使用memcmp比较字符串，要保证count不能超过最短字符串的长度,否则可能出错
			if (memcmp((char *)&g_zbnodearray.node[i].macaddr[4], (char *)&mac[4], 4) == 0)
			{
				g_zbnodearray.node[i].heartbeattime = getcurrenttime();//将当前时间作为心跳时间
				ret = TRUE;
				break;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ret;
}

void HandlezbnodeState()//该函数未使用
{
	int i;
	pzbnode ptempnode = NULL;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		ptempnode = &g_zbnodearray.node[i];
		if (g_zbnodearray.userflag[i] != 0)
		{
	       // if(ptempnode->zbnodelive_val>=ZB_OFF_LINE)
	       if(ptempnode->testalivecount >= ZB_TESTALIVE_MAXCOUNT)
	        { 
	            if(ptempnode->socket > 0)
             	{
             		printf("delete node!!!!!!!!\r\n");
             		close(ptempnode->socket);
					memset(ptempnode, 0, sizeof(zbnode));
             	}
				 g_zbnodearray.userflag[i] = 0;
	        }
		    
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ;
	

}

int getsocketbymac(char *mac)//该函数未使用
{
	int i;
	int socket = 0;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if ((g_zbnodearray.userflag[i] != 0) && (memcmp(g_zbnodearray.node[i].macaddr, mac, 8) == 0))
		{
			socket = g_zbnodearray.node[i].socket;
			break;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return socket;
}
void SaveZigbeeNode(void)
{
		                                                                                   
	int i;
	int index = 0;
	char tempbuf[1024] = {0};
	pzbnode node;
  	int fd = open("/var/log/zigbeenode", O_CREAT | O_TRUNC | O_RDWR, 0666);//O_TRUNC若文件存在，则长度被截为0，属性不变
  	if(fd < 0)
  	{
   	 	perror("open led_status.log failed"); 
  	}
  	else
  	{
  		printf("SaveZigbeeNode to /var/log/zigbeenode!!!\r\n");
	  	for (i = 0; i < MAXZBNODECOUNT; i++)
		{
			if(g_zbnodearray.userflag[i] != 0)
			{
				node=(&g_zbnodearray.node[i]);
				sprintf(tempbuf, "node[%d] mac addr=0x%02X%02X%02X%02X%02X%02X%02X%02X, socketid=%d loginflag=%d alive=%d\n", index++,
				node->macaddr[0], node->macaddr[1], node->macaddr[2], node->macaddr[3], node->macaddr[4],
				node->macaddr[5], node->macaddr[6], node->macaddr[7], node->socket, node->loginflag, node->zbnodelive_val);
	     		write(fd, tempbuf, strlen(tempbuf));   
			}
		}
  	}      
	fsync(fd); //系统调用fsync将所有已写入文件描述符fd的数据真正的写到磁盘或者其他下层设备上
  	close(fd); 
}

bool StrtoBCD(char *str, int strlen,unsigned char *BCD)
{
	if(str == NULL)
	{
		return false;
	}
	int tmp = strlen;
	if ((tmp % 2) != 0)
	{
		return false;
	}
	tmp -= tmp % 2;
	if(tmp == 0)
	{
		return false;
	}
	int i, j;
	for(i = 0; i < tmp; i++)
	{
		if(str == 0 || !(str[i] >= '0' && str[i] <= '9' || str[i] >= 'a' && str[i] <= 'f' || str[i] >= 'A' && str[i] <= 'F'))
		{
			return false;
		}
	}	 
	for(i = 0,j = 0; i < tmp / 2; i++,j += 2)
	{
		if(str[j] > '9')
		{
			str[j] > 'F' ? (BCD[i] = str[j] - 'a' + 10): (BCD[i] = str[j] - 'A' + 10);	
		}
		else
		{
			BCD[i] = str[j] - '0';
		}
		if(str[j + 1] > '9')
		{
			str[j + 1] > 'F' ? (BCD[i] = (BCD[i] << 4) + str[j + 1]- 'a' + 10):(BCD[i] = (BCD[i] << 4) + str[j + 1] - 'A' + 10);	
		}
		else
		{
			BCD[i] = (BCD[i] << 4) + str[j + 1] - '0'; 
		}      
	}
	return true;    
}


void NodeTableHandle(char *databuf, int databuflen)
{
	char *p = NULL;
	int index = 0;
	int templen = 0;
	zbnode zb_vals;
	
	memset(&zb_vals, 0, sizeof(zbnode));
	zb_vals.zbnodelive_val = 0;
	zb_vals.testtime = getcurrenttime();
	zb_vals.heartbeatinterval = HEARTBEATINTERVAL;//5s

	p = strstr(databuf, "Node Table");
	
	if (p == NULL)
	{
		return;
	}
	p += strlen("Node Table");
	char bufindex[10];
	char agebuf[10];
	int age = 0;
	//000D6F00
	char mac[8];
	mac[0] = 0x00;
	mac[1] = 0x0D;
	mac[2] = 0x6F;
	mac[3] = 0x00;

	while(1)
	{	
		sprintf(bufindex, "%02X", index);
		char *temp = strstr(p, bufindex);
		if (temp == NULL)
		{
			break;
		}
		p = temp;
		StrtoBCD(temp + 3, 8, mac + 4);
		memcpy(zb_vals.macaddr, mac, 8);
		printhex(temp + 3, 8);
		
		printhex(mac, 8);
		memset(agebuf, 0, sizeof(agebuf));
		StrtoBCD(temp + 16, 2, agebuf);
		age = agebuf[0];
		printf("NodeTableHandle node index=%d age=%d mac=0x", index, age);
		printhex(mac, 8);
		printf("\r\n");
		if (isExistNode(mac) == FALSE)
		{
			//nodearray_print();	
			zb_vals.testtime = zb_vals.testtime - index;
			zb_vals.heartbeattime = zb_vals.testtime - index;
			zb_vals.socketstarttime = zb_vals.testtime - index;

			if (g_initflag == 1)
			{
				zb_vals.zbnodelive_val = 0;		
				//addNewzbNode((void*)&zb_vals);
			}
			else
			{
				//zb_vals.zbnodelive_val = age;
			}				
		}
		else 
		{
			updatezbnodelive_val(zb_vals.macaddr, age);	
		}
		p += 20;
		index++;
	}
	g_initflag = 0;
}

int getzigbeecmd(char *zbcmd, char *omccmd)//该函数未使用
{
	int i;
	int len = 0;
	struct  Lamp_Info tmplampinfo;
	
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0) 
		{
			memcpy(tmplampinfo.lamp_id, g_zbnodearray.node[i].macaddr + 4, 4);
			memcpy((char*)&tmplampinfo + 4, omccmd, 4);
			memcpy(zbcmd + len, (char*)&tmplampinfo, sizeof(tmplampinfo));
			len += 8;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return len;
}
int getzigbeeMAC(char *zbcmd)//该函数未使用
{
	int i;
	int len = 0;

	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0) 
		{
			memcpy(zbcmd + len, g_zbnodearray.node[i].macaddr + 4, 4);
			len += 4;
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	len = 4;
	return len;
}


void check_light_status_answer(char *str)//检查灯具的状态响应函数
{
	int i;
	int  ret = 0;
	zbnode *p = (zbnode *)str;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] !=0)
		{
			if(g_zbnodearray.node[i].resend_cmd_flag != 1)continue;
			if (memcmp((char *)&g_zbnodearray.node[i].macaddr[4], (char *)&p->macaddr[4], 8) == 0)
			{
				printf("******check_light_status_answer success...******\n");
				if(g_zbnodearray.node[i].lamp_type == 1)//单调色温
				{
					if(g_zbnodearray.node[i].bringhtness != p->bringhtness);
					else if(g_zbnodearray.node[i].colortemp != p->colortemp);
					else g_zbnodearray.node[i].resend_cmd_flag = 0;
				}
				else if(g_zbnodearray.node[i].lamp_type == 2)//单调RGB
				{
					if(g_zbnodearray.node[i].lightness != p->lightness);
					else if(g_zbnodearray.node[i].red != p->red);
					else if(g_zbnodearray.node[i].green != p->green);
					else if(g_zbnodearray.node[i].blue != p->blue);
					else g_zbnodearray.node[i].resend_cmd_flag = 0;
				}
				else if(g_zbnodearray.node[i].lamp_type == 3)//色温和RGB都调
				{
					if(g_zbnodearray.node[i].lightness != p->lightness);
					//else if(g_zbnodearray.node[i].colortemp!=p->colortemp);
					else if(g_zbnodearray.node[i].red != p->red);
					else if(g_zbnodearray.node[i].green != p->green);
					else if(g_zbnodearray.node[i].blue != p->blue);
					else g_zbnodearray.node[i].resend_cmd_flag = 0;
				}
				break;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}

void zigbee_node_heratbeat_outtime(long time)//心跳超时函数
{
	int i;
	int  ret = 0;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)//100
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			if((time - g_zbnodearray.node[i].heartbeattime) > 150)//说明没有连接上,超时了
			{
				//由mac地址区分是哪个灯登录超时，然后将该灯的标志位置0
				send_lamp_loginout((unsigned char *)g_zbnodearray.node[i].macaddr, g_zbnodearray.node[i].lamp_type);
				g_zbnodearray.userflag[i] = 0;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}

//添加zigbee节点的亮度参数
unsigned char add_zbnode_light_parm(char *mac,uint8 brightness,uint8 colortemp,uint8 r,uint8 g,uint8 b,uint8 flag)
{
    int i;
	unsigned char ret = 0;
	char temp_mac[8] = {0};
	//temp_mac[0] = 0;
	//temp_mac[1] = 0x0D;
	//temp_mac[2] = 0x6F;
	//temp_mac[3] = 0;
	memcpy((char *)&temp_mac[4], mac, 4);
	pthread_mutex_lock(&nodearraymutex);
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			if (memcmp((char *)&g_zbnodearray.node[i].macaddr[4], (char *)&temp_mac[4], 4) == 0)
			{
				if(flag & BRIGHTNESS)g_zbnodearray.node[i].bringhtness = brightness, g_zbnodearray.node[i].lightness = brightness;
				if(flag & COLORTEMP)g_zbnodearray.node[i].colortemp = colortemp;
				if(flag & R_G_B)
				{
					g_zbnodearray.node[i].red = r;
					g_zbnodearray.node[i].green = g;
					g_zbnodearray.node[i].blue = b;
				}
				g_zbnodearray.node[i].resend_cmd_flag = 1;//调光时用到的标志位
				ret = g_zbnodearray.node[i].lamp_type;//灯的类型  0：亮度 1: 色温；2：RGB;  3：RGB+colortem         
				break;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ret;
}

int read_lamp_info(struct LampInfo temp)
{
	int i;
	int  ret = 0;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			if (memcmp(g_zbnodearray.node[i].macaddr, temp.mac, 8) == 0)//如果mac地址匹配正确
			{
				temp.brightness = g_zbnodearray.node[i].bringhtness;
				temp.colortemp  = g_zbnodearray.node[i].colortemp;
				temp.lightness  = g_zbnodearray.node[i].lightness;
				temp.valu_r = g_zbnodearray.node[i].red;
				temp.valu_g = g_zbnodearray.node[i].green;
				temp.valu_b = g_zbnodearray.node[i].blue;
				ret = 1;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return ret;
}

void check_login_flag(uint8 flag)
{
	int i = 0;
	pthread_mutex_lock(&nodearraymutex);
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if(g_zbnodearray.userflag[i] > 0)
		{
			if(g_zbnodearray.node[i].loginflag == 2)
			{
				if(flag == 1 && g_zbnodearray.node[i].lamp_type < TYPE_PAD)g_zbnodearray.node[i].loginflag = 3;
				if(flag == 0 && g_zbnodearray.node[i].lamp_type >= TYPE_PAD)g_zbnodearray.node[i].loginflag = 3;
			}
		} 
	}
	pthread_mutex_unlock(&nodearraymutex);	
	return;
}

void send_lamps_info_to_windows_client(void)
{
	int i = 0, count = 1;
	unsigned char buf[1024] = {0};
	buf[0] = 0x7B;
	buf[1] = 0;
	buf[2] = 0x02;
	pthread_mutex_lock(&nodearraymutex);
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if(g_zbnodearray.userflag[i] > 0)
		{
			buf[4 + (count - 1)*8] = count;
			buf[4 + (count - 1)*8 + 1] = g_zbnodearray.node[i].neighbor[0].cRSSI;//节点个数-->灯连接的个数
			buf[4 + (count - 1)*8 + 2] = g_zbnodearray.node[i].loginflag;//登录标志位
			buf[4 + (count - 1)*8 + 3] = g_zbnodearray.node[i].lamp_type;//类型RGB等
			buf[4 + (count - 1)*8 + 4] = g_zbnodearray.node[i].macaddr[4];
			buf[4 + (count - 1)*8 + 5] = g_zbnodearray.node[i].macaddr[5];
			buf[4 + (count - 1)*8 + 6] = g_zbnodearray.node[i].macaddr[6];
			buf[4 + (count - 1)*8 + 7] = g_zbnodearray.node[i].macaddr[7];
			count++;
		} 
	}
	if(count != 1)
	{
		buf[3] = count - 1;
		buf[3 + (count - 1)*8 + 8] = 0x7D;
		tcp_send(fd_A[0], buf, 3 + (count - 1)*8 + 8 + 1);//返回给QT进行显示
	}
	pthread_mutex_unlock(&nodearraymutex);	
	return;
}
void send_server_info_to_windows_client(void)//将信息返回给QT
{
	unsigned char buf[1024] = {0};
    char g_version [] = "V2.1.20170525"; 
    int len = strlen(g_version);
	buf[0] = 0x7B;
	buf[1] = 0;
	buf[2] = 0x04;
	buf[3] = g_zbnetworkparam.channel;
	memcpy((unsigned char *)&buf[4], (unsigned char *)g_zbnetworkparam.panid, 8);
	memcpy((unsigned char *)&buf[12], (unsigned char *)g_zbomcparam.serverip, 14);
    memcpy((unsigned char *)&buf[26], g_version, len);
	tcp_send(fd_A[0], buf, 26+len+1);//返回给QT进行显示
}

void set_lamp_scene(char *str,uint8 key)//设置灯的情景参数
{
	int i;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			if (memcmp((char *)&g_zbnodearray.node[i].macaddr[4], str, 4) == 0)
			{
				if(g_zbnodearray.node[i].scene_key == key)break;
				else g_zbnodearray.node[i].scene_key = key;//触发的键值
				/*resend_cmd_flag=1-->调光需要重发  resend_cmd_flag=2-->情景需要重发*/
				g_zbnodearray.node[i].resend_cmd_flag = 2;
				break;
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}

void check_lamp_scene(char *str,uint8 key)//检查灯的情景
{
	int i;
	pthread_mutex_lock(&nodearraymutex); 
	for(i = 0; i < MAXZBNODECOUNT; i++)
	{
		if (g_zbnodearray.userflag[i] != 0)
		{
			if (memcmp((char *)&g_zbnodearray.node[i].macaddr[4], str, 4) == 0)
			{
				if(g_zbnodearray.node[i].scene_key == key)
				{
					g_zbnodearray.node[i].scene_key = 0;
					g_zbnodearray.node[i].resend_cmd_flag = 0;
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&nodearraymutex); 
	return;
}



