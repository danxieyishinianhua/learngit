#ifndef _TCP_RS232_H_
#define _TCP_RS232_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <memory.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <netdb.h> 
#include <sys/select.h>  
#include <net/if.h>  
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "INLcpMsg.h"
#include "nodearray.h"


#define BUF_SIZE 1024  
#define RS232_BAUDRATE 	 115200
#define RS232_DEVICE 	 "/dev/ttyATH0"
#define LOG_FILE_PATH 	 "/tmp/led_ctrl.log"
#define HEXLEN 		3


int rs232_fd;


char Version[16];
char log_buf[1024];

pthread_mutex_t mutex_list;
pthread_cond_t cond_list;


struct sockaddr_in addr;  //socket connect

pthread_t pth_rs232_read;
pthread_t pth_rs232_write;
pthread_t pth_tcp_recv;
pthread_t pth_link;
pthread_t pth_zb_live_check;
pthread_t pth_tcpserver;
pthread_t pth_zigbeenetworking;
pthread_t pth_rs232_resend;


void *tcp_recv(void *arg);
void *check_link	(void *arg);
void *rs232_recv	(void *arg);
void *rs232_write_list(void *arg);
void *zb_live_check	(void *arg);
void *zigbeenetworking(void *arg);
void *rs232_resend(void *arg);;

void zb_tree_insert(void *data);
int zigbee_check(void *data,void* list_data);
int get_loacl_time(char *local_time);
void get_server_msg(void);
char get_mac	(char *buf, int size);
char tcp_send(int socket,char *msg, int size);
char rs232_send(char *data, int datalen);
void log_to_file(const char *str1, char *str2, char *str3);
void HexToStr(const uint8 *digest,uint8 (*buff)[HEXLEN],uint32 len);
uint8 StrToHex(char hight_ch,char low_ch);
int pox_system(const char * cmd_line);
long getcurrenttime();
void printhex(uint8 *databuf, int datalen);
int InitZigbee(ZBNETWORKPARAM *zbnetworkparam );

#endif   //__SG_RS232_UART_H__
