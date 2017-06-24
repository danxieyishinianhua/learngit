#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include "tcp_rs232.h"
#include "send_list.h"

#define LOCALSERVERPORT 6000    // the port users will be connecting to  
#define BACKLOG 1     // how many pending connections queue will hold  

int fd_A[BACKLOG];    // accepted connection fd  1
int conn_amount;      // current connection amount  
extern int g_initflag ;
int senddebugdataresponse(char *data, int len)
{
	if (data != NULL)
	{
		if(fd_A[0] > 0)
		{
			printf("senddebugdataresponse len=%d\r\n", len);
			printhex(data, len);
			send(fd_A[0], data, len, 0);//send�����ǰ�data����copy��fd_A[0]���ͻ�������ʣ��ռ���
		}
		else
		{
			printf("senddebugdataresponse fd_A[0]=%d\r\n", fd_A[0]);
		}
	}
	
}
char IsZigbeeDebug()
{
	if (fd_A[0] > 0)
	{
		return 1;
	}
	return 0;
}
void showclient()  
{  
    int i;  
    printf("client amount: %d\r\n", conn_amount);  
    for (i = 0; i < BACKLOG; i++) {  
        printf("client[%d]:%d  ", i, fd_A[i]);  
    }  
    printf("\r\n");  
}  

void *tcpserverhandle(void *arg)
{  
    int sock_fd, new_fd;             // listen on sock_fd, new connection on new_fd  
    struct sockaddr_in server_addr;  // server address information  
    struct sockaddr_in client_addr;  // connector's address information  
    socklen_t sin_size;  
    int yes = 1;  
    char buf[BUF_SIZE];  
    int ret;  
    int i;  
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) //�������ؼ����׽���
	{  
        perror("socket");  
        exit(1);  
    }
	//�����׽��ֵ�����ʹ���ܹ��ڼ����������ʱ������ٴ�ʹ���׽��ֵĶ˿ں�IP 
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {  
        perror("setsockopt");  
        exit(1);  
    }  
    server_addr.sin_family = AF_INET;         // host byte order  
    server_addr.sin_port = htons(LOCALSERVERPORT);     // short, network byte order-->6000
    server_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP  
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));  
	//���׽���
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {  
        perror("bind");  
        exit(1);  
    }
	//���ü���
    if (listen(sock_fd, BACKLOG) == -1) {  
        perror("listen");  
        exit(1);  
    }  
    printf("listen port %d\r\n", LOCALSERVERPORT);  
    fd_set fdsr;  
    int maxsock;  
    struct timeval tv;  
    conn_amount = 0;  
    sin_size = sizeof(client_addr);  
    maxsock = sock_fd;  
    while (1)   
    {  
        // initialize file descriptor set  
        FD_ZERO(&fdsr);  
        FD_SET(sock_fd, &fdsr);  // add fd  to fdsr
        // timeout setting  
        tv.tv_sec = 10;//��30s�޸�Ϊ10s,�����������޸�
        tv.tv_usec = 0;  
        // add active connection to fd set  
        for (i = 0; i < BACKLOG; i++) 
		{  
            if (fd_A[i] != 0) 
			{  
                FD_SET(fd_A[i], &fdsr); //��fd_A[0]���õ�fdsr������  new_fd
            }  
        }  
        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);//select��10sʱ������������ʱʱ��֮�����¼������ͷ����ˣ������ڳ�ʱ�󲻹�����һ������ 
        if (ret < 0) 
		{         
            perror("select"); // error    
            break;  
        }
        else if (ret == 0) 
        { 
            printf("timeout\r\n"); // time out    
            continue;  
        }  
        // check every fd in the set  
        for (i = 0; i < conn_amount; i++)   
        {  
            if (FD_ISSET(fd_A[i], &fdsr)) // check which fd is ready  
            {  
                ret = recv(fd_A[i], buf, sizeof(buf), 0); //����fd_A[0]�ļ�����Ϣ����buf -->���ܿͻ����е���Ϣ
                if (ret <= 0)//�ļ���û����Ϣ
		        {   
                    printf("ret : %d and client[%d] close\r\n", ret, i);  
                    close(fd_A[i]);  
                    FD_CLR(fd_A[i], &fdsr);  // delete fd from fdsr  
                    fd_A[i] = 0; //����
                    conn_amount--;  
                }  
                else // receive data    
                {        
                   if (ret < BUF_SIZE)  
                   {
                    	printf("client[%d] receive data and send to serial,datalen=%d\r\n", i, ret);//����QT���͵���Ϣ
						printhex(buf, ret);//��ӡQT���͵���Ϣ
						//buf�洢�������Լ������
						if(buf[0] == 0x7B)
						{
							if(buf[2] == 0x01)
							{
								send_lamps_info_to_windows_client();//QTд�Ľ��淢�������ؽ��ղ��������Ϣ���ظ�QT
							}
							else if(buf[2] == 0x02)
							{
								if(buf[3] == 0x01)
								{
									char channelbuf[4][10] = {{"686E518C"}, {"686E518D"}, {"686E518E"}, {"686E518F"}};
									unsigned char ch_buf[4] = {11, 15, 20, 25};
									char strbuff[50] = {0};
									//uci set...�������õ���Ϣ���뵽openwrt��һ�������ļ���
									sprintf(strbuff, "uci set zigbee.@param[0].PanID='%s'", (char *)&channelbuf[buf[4]%4][0]);
									system(strbuff);
									memset(strbuff, 0, 50);
									sprintf(strbuff, "uci set zigbee.@param[0].Channel=%d", ch_buf[buf[4]/4]);
									system(strbuff);	
								}
								else if(buf[3] == 0x02)
								{
									if(buf[4] == 0)system("uci set zigbee.@param[0].ServerIP=112.74.198.150");
									else if(buf[4] == 1) system("uci set zigbee.@param[0].ServerIP=192.168.8.8");
								}
								else if(buf[3] == 0x03)
								{
									if(buf[4] == 1)system("uci set zigbee.@param[0].networkenable=1");
									else system("uci set zigbee.@param[0].networkenable=0");
								}
								system("uci commit zigbee"); //д������
							}
							else if(buf[2] == 0x03)
							{
								system("reboot");
							}
							else if(buf[2] == 0x04)
							{
								ReadOMCConfig();
								send_server_info_to_windows_client();//QTд�Ľ��淢�������ؽ��ղ��������Ϣ���ظ�QT
							}
						}
						else//buf[0]!=0x7Bʱ�ͽ���Ϣд�뵽�����ļ��п��Ƶƾ߹���
						{
							msg_write_to_list(buf, ret);//д���ļ���Ȼ�󷢸�����
						}
					} 
                }  
            }  
        }  
        // check whether a new connection comes  
        if (FD_ISSET(sock_fd, &fdsr))  // accept new connection   
        {  
            new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size); //���ܿͻ�������
            if (new_fd <= 0)  
            {  
                perror("accept");  
                continue;  
            }  
            // add to fd queue  
            if (conn_amount < BACKLOG)   
            {  
                fd_A[conn_amount++] = new_fd; //conn_amount=1;
                printf("new connection client[%d] %s:%d\r\n", conn_amount,  
                        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));  
                if (new_fd > maxsock)  // update the maxsock fd for select function  
                    maxsock = new_fd;  
            }  
            else   
            {  
                printf("max connections arrive, old connect exit\r\n");  
                send(fd_A[0], "good bye", 9, 0);  
                close(fd_A[0]);
				fd_A[0] = new_fd;   
                if (new_fd > maxsock)  // update the maxsock fd for select function  
                    maxsock = new_fd;  
            }  
        }  
        showclient();  
    }  
    // close other connections  
    for (i = 0; i < BACKLOG; i++)   
    {  
        if (fd_A[i] != 0)   
        {  
            close(fd_A[i]);  
        }  
    }  
}  
