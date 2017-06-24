#include "serial.h"
#include "INLcpMsg.h"

int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
	    B38400, B19200, B9600, B4800, B2400, B1200, B300,B115200, };
int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300,
	    38400,  19200,  9600, 4800, 2400, 1200,  300, 115200,};

void set_speed(int fd, int speed)
{
	unsigned   i;
	int   status;
	struct termios   Opt;
	tcgetattr(fd, &Opt);
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
	{
		if  (speed == name_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			Opt.c_lflag &= ~ICANON;
			Opt.c_oflag &= ~OPOST; 
			Opt.c_iflag &= ~(IXON | IXOFF | IXANY);
			Opt.c_iflag &= ~(INLCR | IGNCR | ICRNL);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if  (status != 0)
				perror("tcsetattr fd1");
			return;
		}
		tcflush(fd,TCIOFLUSH);
	}
}
/**
 *@brief   parity checking  
 */
int set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options;
	if  ( tcgetattr( fd, &options)  !=  0)
	{
		perror("SetupSerial 1");
		return(FALSE);
	}
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;//add........
	switch (databits) /*set data bits*/
	{
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr,"Unsupported data size\n");
		return (FALSE);
	}
	switch (parity)
	{
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* set even parity check */ 
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* set Odd parity check */  
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported parity\n");
		return (FALSE);
	}
	/* set stop bits  */   
	switch (stopbits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported stop bits\n");
		return (FALSE);
	}
	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;
	options.c_cc[VTIME] = 1;//150; // 15 seconds
	options.c_cc[VMIN] = 0;

	tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;   /*Output*/
	options.c_oflag &=~(INLCR|IGNCR|ICRNL);
	options.c_oflag &=~(ONLCR|OCRNL);

	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}
/**
 *@breif open Serial Port
 */
int OpenDev(char *Dev)
{
	int	fd = open( Dev, O_RDWR | O_NOCTTY);         //| O_NOCTTY | O_NDELAY O_NONBLOCK
	if (-1 == fd)
	{ 
		perror("Can't Open Serial Port");
		return -1;
	}
	else
		return fd;

}



int uart_setup(char *dev, int baud)
{
	int fd;
	fd = OpenDev(dev);
	if (fd > 0)
	{
		set_speed(fd, baud);
	}
	else
	{
		printf("Can't Open Serial Port!\n");
		return -1;
	}
	if (set_Parity(fd, 8, 1, 'N')== FALSE)
	{
		printf("Set Parity Error\n");
		return -1;
	}

	return fd;
}


int uart_close(int fd)
{
	if (fd)
		close(fd);

	fd = 0;
	return 0;
}

#if 0
int uart_read_byte(int uart_index, uint8_t *data) 
{

	char ch;
	read(fd,&ch,1);
	*data=ch;

	return 1;
}
void uart_write_byte(int uart_index, uint8_t ch)
{
	write(fd, &ch, 1);
}
#endif
