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
			send(fd_A[0], data, len, 0);//send仅仅是把data数据copy到fd_A[0]发送缓冲区的剩余空间里
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
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) //创建本地监听套接字
	{  
        perror("socket");  
        exit(1);  
    }
	//设置套接字的属性使它能够在计算机重启的时候可以再次使用套接字的端口和IP 
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {  
        perror("setsockopt");  
        exit(1);  
    }  
    server_addr.sin_family = AF_INET;         // host byte order  
    server_addr.sin_port = htons(LOCALSERVERPORT);     // short, network byte order-->6000
    server_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP  
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));  
	//绑定套接字
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {  
        perror("bind");  
        exit(1);  
    }
	//设置监听
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
        tv.tv_sec = 10;//由30s修改为10s,有问题重新修改
        tv.tv_usec = 0;  
        // add active connection to fd set  
        for (i = 0; i < BACKLOG; i++) 
		{  
            if (fd_A[i] != 0) 
			{  
                FD_SET(fd_A[i], &fdsr); //将fd_A[0]设置到fdsr集合中  new_fd
            }  
        }  
        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);//select在10s时间内阻塞，超时时间之内有事件到来就返回了，否则在超时后不管怎样一定返回 
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
                ret = recv(fd_A[i], buf, sizeof(buf), 0); //接收fd_A[0]文件中消息存入buf -->接受客户端中的消息
                if (ret <= 0)//文件中没有信息
		        {   
                    printf("ret : %d and client[%d] close\r\n", ret, i);  
                    close(fd_A[i]);  
                    FD_CLR(fd_A[i], &fdsr);  // delete fd from fdsr  
                    fd_A[i] = 0; //清零
                    conn_amount--;  
                }  
                else // receive data    
                {        
                   if (ret < BUF_SIZE)  
                   {
                    	printf("client[%d] receive data and send to serial,datalen=%d\r\n", i, ret);//接收QT发送的消息
						printhex(buf, ret);//打印QT发送的消息
						//buf存储内容是自己定义的
						if(buf[0] == 0x7B)
						{
							if(buf[2] == 0x01)
							{
								send_lamps_info_to_windows_client();//QT写的界面发送由网关接收并将获得信息返回给QT
							}
							else if(buf[2] == 0x02)
							{
								if(buf[3] == 0x01)
								{
									char channelbuf[4][10] = {{"686E518C"}, {"686E518D"}, {"686E518E"}, {"686E518F"}};
									unsigned char ch_buf[4] = {11, 15, 20, 25};
									char strbuff[50] = {0};
									//uci set...命令设置的信息存入到openwrt的一个配置文件中
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
								system("uci commit zigbee"); //写入配置
							}
							else if(buf[2] == 0x03)
							{
								system("reboot");
							}
							else if(buf[2] == 0x04)
							{
								ReadOMCConfig();
								send_server_info_to_windows_client();//QT写的界面发送由网关接收并将获得信息返回给QT
							}
						}
						else//buf[0]!=0x7B时就将信息写入到串口文件中控制灯具工作
						{
							msg_write_to_list(buf, ret);//写在文件中然后发给串口
						}
					} 
                }  
            }  
        }  
        // check whether a new connection comes  
        if (FD_ISSET(sock_fd, &fdsr))  // accept new connection   
        {  
            new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size); //接受客户端连接
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
