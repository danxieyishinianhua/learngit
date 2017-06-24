#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  
#include <fcntl.h>     
#include <termios.h>    
#include <errno.h>  



void set_speed(int fd, int speed);

//void set_Parity(int fd, int databits, int stopbits, int parity);

int OpenDev(char *Dev);

int uart_setup(char *dev, int baud);

int uart_close(int fd);

#endif

