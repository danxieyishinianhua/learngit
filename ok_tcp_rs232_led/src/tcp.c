#include <sys/select.h>
#include "tcp_rs232.h"


void *tcp_recv(void *arg)
{
	
	char tcp_buf[2048];
	int tcpbuflen = 0;
	fd_set socketrx_set;
	struct timeval timeout_recv;
 	FD_ZERO(&socketrx_set);//����ļ�����
 	int maxsocket = 0;

	while(1)
	{
		if (sockfd_server <= 0)
		{
			usleep(1000000);
			printf("tcp recv sockfd_server < 0\n");
			continue;
		}
		FD_ZERO(&socketrx_set);//����ļ�����
		FD_SET(sockfd_server, &socketrx_set);
		maxsocket = sockfd_server;
		timeout_recv.tv_sec  = 2;
		timeout_recv.tv_usec = 0;
		int ret = select(maxsocket + 1, &socketrx_set, NULL, NULL, &timeout_recv);//select��2sʱ������������ʱʱ��֮�����¼������ͷ����ˣ������ڳ�ʱ�󲻹�����һ������  
		if(ret < 0) 
		{
			perror("select");
			printf("select error maxsocket=%d\r\n", maxsocket);
		}
		
		else if(ret == 0) 
		{
			printf("tcp receive timeout\r\n"); 
			continue;
		}
		else 
		{ 
			if(FD_ISSET(sockfd_server, &socketrx_set))//���sockfd_server�Ƿ���socketrx_set������
			{
				memset(tcp_buf, 0, sizeof(tcp_buf));
				tcpbuflen = recv(sockfd_server, (char*)tcp_buf, sizeof(tcp_buf), 0);//���ļ���ȡ������Ϣ�浽tcp_buf
				printf("receive LCPServer data, sockfd_server=%d, data len=%d\r\n ", sockfd_server, tcpbuflen);
				if(tcpbuflen <= 0)
				{
					close(sockfd_server);
					sockfd_server = -1;
					continue;
				}
				#ifdef DEBUG
				printhex(tcp_buf, tcpbuflen);
				#endif
				tcp_recive_unpack(tcp_buf, tcpbuflen);
			}
		}
		usleep(200000);
	}
	memset(log_buf, '\0', sizeof(log_buf));
	sprintf(log_buf, "pthread recv() exit ERROR :errno=%d -> %s", errno, strerror(errno));
	log_to_file(__func__, log_buf, NULL);
	pthread_exit(0);
}


char tcp_send (int sockfd,char *msg, int size)
{
	int ret = 0;
	if(sockfd <= 0)return -1;
	if( (ret = send(sockfd, msg, size, 0)) < 0 )//�ͻ�����һ����send������������������󣬶���������ͨ����send��������ͻ�������Ӧ��
	{
	#ifdef DEBUG
	perror("send");
	#endif
		memset(log_buf, '\0', sizeof(log_buf));
		snprintf(log_buf, sizeof(log_buf), "ERROR   %s", strerror(errno));
		log_to_file(__func__, log_buf, NULL);
	}
	return ret;
}



