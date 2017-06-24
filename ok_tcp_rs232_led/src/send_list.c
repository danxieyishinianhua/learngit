#include "send_list.h"
#include "tcp_rs232.h"
#include "fun_msgpack.h"


struct uart_send_buf rs232_send_list[SEND_BUFF_SIZE + 1];//51

unsigned char rs232_read_current = 0, rs232_write_current = 0;

void msg_write_to_list(unsigned char *date,unsigned char len)
{
	unsigned char parket_len = rs232_send_list[rs232_write_current].datelen;
	
	pthread_mutex_lock(&mutex_list); //用了锁的话全局变量也被锁住了，只能它用完别人才能用
	if((parket_len + len) <= MAX_UART_SEND_LEN)//如果当前写的够这次写入，则插入当前写的parket，不够则写入下一parket
	{
		#ifdef SEND_LIST_DEBUG
		printf("write to list num:%d, list_buff len:%d  ", rs232_write_current, parket_len);
		printf("write date len:%d\n", len);
		printhex(date, len);
		#endif
		memcpy((unsigned char *)&rs232_send_list[rs232_write_current].date_buf[parket_len], date, len);
		rs232_send_list[rs232_write_current].datelen = parket_len + len;
	}
	else if(len > MAX_UART_SEND_LEN)
	{
		printf("write msg is too big, this is send of no waiting\n");
		printhex(date, len);
		rs232_send(date, len);
	}
	else
	{
		//如果当前写的那一行到最后一行了，又从第一个数组开始存储数据
		if(rs232_write_current == SEND_BUFF_SIZE)rs232_write_current = 0;
		else rs232_write_current++;
		
		if(rs232_send_list[rs232_write_current].datelen != 0)
		{
			printf("write_to_list is full, this is send of no waiting\n");
			printhex(date, len);
			rs232_send(date, len);
		}
		else 
		{
			#ifdef SEND_LIST_DEBUG
			printf("write to list num:%d, list_buff len:%d", rs232_write_current, rs232_send_list[rs232_write_current].datelen);
			printf("write date len:%d\n",len);
			printhex(date, len);
			#endif
			memcpy((unsigned char *)&rs232_send_list[rs232_write_current].date_buf[0], date, len);
			rs232_send_list[rs232_write_current].datelen = len;
		}
	}
	
	pthread_cond_signal(&cond_list);//用于唤醒pthread_cond_wait函数		
	pthread_mutex_unlock(&mutex_list);
}

unsigned char msg_read_to_list(void)
{
	pthread_mutex_lock(&mutex_list);
	unsigned char parket_len = rs232_send_list[rs232_read_current].datelen;
	unsigned char ret;
	if(parket_len != 0)
	{
		#ifdef SEND_LIST_DEBUG
		printf("read for list num:%d, list_buff len:%d\n", rs232_read_current, parket_len);
		printhex((unsigned char *)&rs232_send_list[rs232_read_current].date_buf, parket_len);
		#endif
		//将数据发送到串口文件中去
		rs232_send((unsigned char *)&rs232_send_list[rs232_read_current].date_buf, parket_len);
		rs232_send_list[rs232_read_current].datelen = 0;

		if(rs232_write_current != rs232_read_current)//如果当前读的缓冲区不等于写的缓冲区
		{		
			if(rs232_read_current == SEND_BUFF_SIZE)rs232_read_current = 0;
			else rs232_read_current++;
			ret = 1;
		}
		else ret = 0;
	}
	else ret = 0;
	pthread_mutex_unlock(&mutex_list);
	return ret;
}

