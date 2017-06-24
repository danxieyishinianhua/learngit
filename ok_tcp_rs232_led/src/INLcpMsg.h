#ifndef _INLCPMSG_H_
#define _INLCPMSG_H_
#define TRUE 1
#define FALSE 0

typedef unsigned char 	uint8;
typedef unsigned short 	uint16;
typedef unsigned short 	USIGN16;
typedef unsigned int	uint32;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int USIGN32;
typedef unsigned char USIGN8;
typedef signed char SIGN8;

#define HEARTBEATINTERVAL 5//15

struct INLcpMsgSet_list 
{
	unsigned long list_id;
	unsigned int datalen;
	char data[92];
};

#define ZB_TESTALIVE_MAXCOUNT 4
#define ZB_OFF_LINE 0x1F
#define ZB_ON_LINE	0x00
#define CHECK_OFF_LINE	0x0
#endif //INLCP_MSG_H_

