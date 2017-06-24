#ifndef _SEND_LIST_H_
#define _SEND_LIST_H_

#include <sys/types.h>
#include <fcntl.h>
#include "fun_msgpack.h"


#define SEND_BUFF_SIZE 50


struct uart_send_buf
{
	unsigned char datelen;
	unsigned char date_buf[MAX_UART_SEND_LEN];//90---每包数据90个字节
};


extern struct uart_send_buf rs232_send_list[SEND_BUFF_SIZE+1];
extern pthread_mutex_t mutex_list;
extern pthread_cond_t cond_list;



void msg_write_to_list(unsigned char *date,unsigned char len);
unsigned char msg_read_to_list(void);


#endif

