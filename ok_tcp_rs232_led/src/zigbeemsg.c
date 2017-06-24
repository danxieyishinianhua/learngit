#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "INLcpMsg.h"
#include "zigbeemsg.h"

int g_socketid=0;


char SendCOMANDWaitforRESP(char *command, int commandlen, unsigned short cmdtype, char *responsebuf)
{
 return 0;
}
char SendCOMANDWaitforRESPex(char *command, int commandlen, unsigned short cmdtype, char *responsebuf)
{

	char retval = 0;

	unsigned char Buffer[1024];
	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;

	unsigned char receivebuf[500];


	memset(receivebuf, 0, sizeof(receivebuf));
	int validpacklen = 0;
	memset(Buffer, 0, sizeof(Buffer));
	receivelen = 0;
	trycount = 0;
	while (trycount < 10)
	{
		memset(receivebuf, 0, sizeof(receivebuf));
		int len = 0;
		fd_set rdfds; /* 先申明一个 fd_set 集合来保存我们要检测的 socket句柄 */
		struct timeval tv; /* 申明一个时间变量来保存时间 */
		int ret; /* 保存返回值 */
		FD_ZERO(&rdfds); /* 用select函数之前先把集合清零 */
		FD_SET(g_socketid, &rdfds); /* 把要检测的句柄socket加入到集合里 */
		tv.tv_sec = 5;
		tv.tv_usec = 500; /* 设置select等待的最大时间为1秒加500毫秒 */
		
		ret = select(g_socketid + 1, &rdfds, NULL, NULL, &tv); /* 检测我们上面设置到集合rdfds里的句柄是否有可读信息 */
		if(ret < 0) perror("select");/* 这说明select函数出错 */
		else if(ret == 0) 
		{
				break;
		//	printf("超时\n"); /* 说明在我们设定的时间值1秒加500毫秒的时间内，socket的状态没有发生变化 */
		}
		else 
		{ /* 说明等待时间还未到1秒加500毫秒，socket的状态发生了变化 */
			//printf("ret=%d\n", ret); /* ret这个返回值记录了发生状态变化的句柄的数目，由于我们只监视了socket这一个句柄，所以这里一定ret=1，如果同时有多个句柄发生变化返回的就是句柄的总和了 */
			/* 这里我们就应该从socket这个句柄里读取数据了，因为select函数已经告诉我们这个句柄里有数据可读 */
			if(FD_ISSET(g_socketid, &rdfds))
			{ /* 先判断一下socket这外被监视的句柄是否真的变成可读的了 */
				/* 读取socket句柄里的数据 */
				len = recv(g_socketid, (char*)receivebuf, sizeof(receivebuf), 0);
				if (len > 0)
				{
				
					receivelen = len;
					break;
				}
			}
		}

		trycount++;
		usleep(100);
	}
	if (receivelen <=0)
	{
		//m_strDataReceived += "fail to execute command!\r\n";
	}	
	else
	{
		int cmd=receivebuf[0]*256+receivebuf[1];
		if(cmdtype == cmd)
		{
			retval =1;
			memcpy(responsebuf, receivebuf, receivelen);
		}
		else
		{
			retval =0;
		}
		//OnparsePacket(validbuf, validpacklen);
	}

	return retval;
}

	
/////////////////////////////////////////////////////////////////////////////
// ZIGBEEDIALOG message handlers  详见RexBee无线通信协议-v2
int packetZigbeedata(unsigned short cmd,char *dstbuf,char *srcbuf, int len, char *dstmac, char *srcmac)
{
	if((dstbuf ==NULL) ||(dstbuf ==srcbuf))
	{
		return -1;
	}
	unsigned char check=0;
    int packetlen = 0;
	EMBERCMDPACKET *headmsg=(EMBERCMDPACKET*)dstbuf;
	headmsg->header = 0x2A;
	headmsg->packetlen=30+len;
	memcpy(headmsg->srtaddr, srcmac,4);
	memcpy(headmsg->dstaddr, dstmac,4);
	headmsg->framecontrol = 0x4188;//0X8841;
	headmsg->clusterid[0] = cmd%256;//0X3820
	headmsg->clusterid[1] = cmd/256;
	memcpy(dstbuf+sizeof(EMBERCMDPACKET),srcbuf, len);
	packetlen =2+30+len;

	int i;
	for (i=2; i<packetlen; i++)
	{
		check += (unsigned char)dstbuf[i];
	}
	
	dstbuf[packetlen++]= check;
	dstbuf[packetlen++]=0x23;//end

	return packetlen;
}
BYTE IsHexChar(char AChar)
{
    BYTE Result = FALSE;

    switch(AChar) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        Result = TRUE;
    	break;
    default:
        Result = FALSE;
    }

    return Result;
}
char GetHexCharValue(const char HexChar)
{
    char Result = 0x00;
    
    switch(HexChar) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        Result = HexChar - '0';
        break;        
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        Result = HexChar - 'A' + 0x0A;
        break;        
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        Result = HexChar - 'a' + 0x0A;
        break;        
    default:
        Result = 0x00;
    }
    
    return Result;
}
int GetHexData(const char *szHex, char *Buffer)
{
    int i;
    int nStrLen;
    int nDataLen;
    BYTE IsHarfByte;

    nStrLen = strlen(szHex);
    nDataLen = 0;
    IsHarfByte = FALSE;
    for (i = 0; i < nStrLen; i++)
    {
        // 这个也可以
        //if (isxdigit(szHex[i]))
        if (IsHexChar(szHex[i]))
        {
            if (!IsHarfByte)
            {
                Buffer[nDataLen] = GetHexCharValue(szHex[i]) << 4;
                IsHarfByte = TRUE;
            }
            else
            {
                Buffer[nDataLen] += GetHexCharValue(szHex[i]);
                nDataLen++;
                IsHarfByte = FALSE;
            }
        }           
    }

    return nDataLen;
}



int GetHexStr(char *dstbuf, char *Buffer, int BufferLen)
{
    int i;
	int len=0;
  
    for(i= 0; i < BufferLen; i++)
    {
        sprintf(dstbuf+len,"%02X ", Buffer[i] & 0xFF);
  		len +=2;
    }

    return 1;
}


int OnButtonZgBasicparam() 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;

	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}


	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=NODESTATUSOD%256;//read index 0x03E9
	tempsendbuf[1]=NODESTATUSOD/256;//read index
	tempsendbuf[2]=ALLPARAM;//read SUB index
	tempsendbuf[3]=0;//read OPT
	len = 4;

	unsigned short cmd = 0x3800|READFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, NODESTATUSOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(READFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf( "invalid command\r\n");

		return -2;
	}
	else if ( receivelen <=0)
	{
		printf( "no response \r\n");
		return -3;
	}
	char aucSoftware_Version [10];
	char aucHardware_Version [10];
	char aucDev_Type [10];
	memset(aucSoftware_Version, 0, sizeof(aucSoftware_Version));
	memset(aucHardware_Version, 0, sizeof(aucHardware_Version));
	memset(aucDev_Type, 0, sizeof(aucDev_Type));

	memcpy(aucSoftware_Version, p+5,4);
	memcpy(aucHardware_Version, p+5+4,4);
	memcpy(aucDev_Type, p+5+8,6);
	printf("software:%s\r\n haredware:%s\r\n devtype:%s\r\n", aucSoftware_Version,aucHardware_Version,aucDev_Type);

	return 0;
	
}

int OnButtonZgnetworkparam() 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];
	//发送0x7f
	int receivelen = 0;
	char *str = NULL;
	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}


	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=NETWORKPARAMOD%256;//read index 0x03E9
	tempsendbuf[1]=NETWORKPARAMOD/256;//read index
	tempsendbuf[2]=ALLPARAM;//read SUB index
	tempsendbuf[3]=0;//read OPT
	len = 4;

	unsigned short cmd = 0x3800|READFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, NETWORKPARAMOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(READFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf( "invalid command\r\n");

		return -2;
	}
	else if ( receivelen <=0)
	{
		printf( "no response \r\n");
		return -3;
	}


	Network_Parameter networkparam;
	memcpy((char*)&networkparam,p+5, sizeof(Network_Parameter));

	char panid[10];
	char channel[10];
	char power[10];
	memset(panid, 0, sizeof(panid));
	memset(channel, 0, sizeof(channel));
	memset(power, 0, sizeof(power));
	p=p+5;
	sprintf(panid, "%X%X%X%X",p[3], p[2], p[1],p[0]);
	sprintf(channel, "%X",p[4]);
	sprintf(power, "%d",p[5]);	
	printf("PANID:%s\r\n channel:%s\r\n power:%s\r\n", panid,channel,power);

	return 0;
	
}

int OnButtonZgGettime() 
{
	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];
	//发送0x7f
	int receivelen = 0;
	char *str = NULL;
	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	

	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=DATETIMEOD%256;//read index 0x03E9
	tempsendbuf[1]=DATETIMEOD/256;//read index
	tempsendbuf[2]=ALLPARAM;//read SUB index
	tempsendbuf[3]=0;//read OPT
	len = 4;

	unsigned short cmd = 0x3800|READFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, DATETIMEOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(READFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf( "invalid command\r\n");

		return -2;
	}
	else if ( receivelen <=0)
	{
		printf( "no response \r\n");
		return -3;
	}

	Date_Time networkparam;
	
	memcpy((char*)&networkparam,p+5, sizeof(Date_Time));


	printf("time:%02d%02d%02d:%02d%02d%02d  ucStatus:%d\r\n", networkparam.ucYear,networkparam.ucMonth, networkparam.ucDate,networkparam.ucHour,
		networkparam.ucMinute,networkparam.ucSecond,networkparam.ucStatus);

	return 0;
		
}

int OnButtonZgRestore() 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;

	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=TRIGGERPARAMOD%256;//read index 0x03E9
	tempsendbuf[1]=TRIGGERPARAMOD/256;//read index
	tempsendbuf[2]=2;//write SUB index
	tempsendbuf[3]=0;
	tempsendbuf[4]=1;
	tempsendbuf[5]=1;
	len = 6;


	unsigned short cmd = 0x3800|WRITEFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, TRIGGERPARAMOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}
	printf("restore device successfully!\r\n");
	TRIGGER_PARAMETER networkparam;
	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	memcpy((char*)&networkparam,p+5, sizeof(TRIGGER_PARAMETER));

	return 0;
		
}
int  OnButtonZgReboot() 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;

	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=TRIGGERPARAMOD%256;//read index 0x03E9
	tempsendbuf[1]=TRIGGERPARAMOD/256;//read index
	tempsendbuf[2]=3;//write SUB index reboot 恢复出厂
	tempsendbuf[3]=0;
	tempsendbuf[4]=1;
	tempsendbuf[5]=1;
	len = 6;


	unsigned short cmd = 0x3800|WRITEFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, TRIGGERPARAMOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}
	
	printf("reset device successfully!\r\n");
	TRIGGER_PARAMETER networkparam;
	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	memcpy((char*)&networkparam,p+5, sizeof(TRIGGER_PARAMETER));

	return 0;
		
}

int  OnButtonZgTransfer(char *srcmac, char *dstmac ,char *buffer, int buflen) 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;
	
	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[600];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	

	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}

	if (buflen>50)
	{
		//MessageBox("数据不能大于50字节!");
		return -1;
	}

	tempsendbuf[0]=COMPORTOD%256;//read index 0x03E9
	tempsendbuf[1]=COMPORTOD/256;//read index
	tempsendbuf[2]=0;//write SUB index
	tempsendbuf[3]=0;


	memcpy(tempsendbuf+5, buffer, buflen);
	len = 5 + buflen;
	tempsendbuf[4] = buflen;


	unsigned short cmd = 0x3800|WRITEFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, COMPORTOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(WRITEFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf( "invalid command\r\n");

		return -2;
	}
	else if ( receivelen <=0)
	{
		printf( "no response \r\n");
		return -3;
	}


	return 0;
		
}

int  OnButtonZgGpio() 
{
	char ret = 0;
	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];
  	//发送0x7f
	int receivelen = 0;
	char *str = NULL;
	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=TRIGGERPARAMOD%256;//read index 0x03E9
	tempsendbuf[1]=TRIGGERPARAMOD/256;//read index
	tempsendbuf[2]=3;//write SUB index reboot 恢复出厂
	tempsendbuf[3]=0;
	tempsendbuf[4]=1;
	tempsendbuf[5]=1;
	len = 6;

	unsigned short cmd = 0x3800|WRITEFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, TRIGGERPARAMOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(WRITEFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf("invalid command\r");
		return -2;
	}
	else if ( receivelen <=0)
	{
		printf("no response\r");
		return -3;
	}
	
	printf("reboot devie successfully!！\r");
	TRIGGER_PARAMETER networkparam;
	memcpy((char*)&networkparam,p+5, sizeof(TRIGGER_PARAMETER));


	//m_data_analysis.Format("时间:%02d%02d%02d:%02d%02d%02d  同步状态:%d\r\n", networkparam.ucYear,networkparam.ucMonth, networkparam.ucDate,networkparam.ucHour,
	//	networkparam.ucMinute,networkparam.ucSecond,networkparam.ucStatus);
	//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);aucDev_Type

	return 0;
		
}

void OnButtonZgGpioread() 
{
	// 
	
}

int  OnButtonZgChildtable() 
{
	char ret = 0;


	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];
 
	//发送0x7f
	int receivelen = 0;
	char *str = NULL;

	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=CHILDTABLE%256;//read index 0x03E9
	tempsendbuf[1]=CHILDTABLE/256;//read index
	tempsendbuf[2]=ALLPARAM;//read SUB index
	tempsendbuf[3]=0;//read OPT
	len = 4;

	unsigned short cmd = 0x3800|READFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, CHILDTABLE,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(READFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf( "invalid command\r\n");

		return -2;
	}
	else if ( receivelen <=0)
	{
		printf( "no response \r\n");
		return -3;
	}
	char aucSoftware_Version [10];
	char aucHardware_Version [10];
	char aucDev_Type [10];
	memset(aucSoftware_Version, 0, sizeof(aucSoftware_Version));
	memset(aucHardware_Version, 0, sizeof(aucHardware_Version));
	memset(aucDev_Type, 0, sizeof(aucDev_Type));

	memcpy(aucSoftware_Version, p+5,4);
	memcpy(aucHardware_Version, p+5+4,4);
	memcpy(aucDev_Type, p+5+8,6);
	//m_data_analysis.Format("软件版本:%s\r\n硬件版本:%s\r\n设备类型:%s\r\n", aucSoftware_Version,aucHardware_Version,aucDev_Type);
	//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);aucDev_Type

	return 0;
	
}

int OnButtonNeighbortable() 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];
	//发送0x7f
	int receivelen = 0;
	char *str = NULL;

	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srcmac[32];
	char dstmac[32];
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=NEIGHBORTABLE%256;//read index 0x03E9
	tempsendbuf[1]=NEIGHBORTABLE/256;//read index
	tempsendbuf[2]=ALLPARAM;//read SUB index
	tempsendbuf[3]=0;//read OPT
	len = 4;

	unsigned short cmd = 0x3800|READFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, NEIGHBORTABLE,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(READFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf("invalid command\r");
		return -2;
	}
	else if ( receivelen <=0)
	{
		printf("no response\r");
		return -3;
	}
	char aucSoftware_Version [10];
	char aucHardware_Version [10];
	char aucDev_Type [10];
	memset(aucSoftware_Version, 0, sizeof(aucSoftware_Version));
	memset(aucHardware_Version, 0, sizeof(aucHardware_Version));
	memset(aucDev_Type, 0, sizeof(aucDev_Type));

	memcpy(aucSoftware_Version, p+5,4);
	memcpy(aucHardware_Version, p+5+4,4);
	memcpy(aucDev_Type, p+5+8,6);
	//m_data_analysis.Format("软件版本:%s\r\n硬件版本:%s\r\n设备类型:%s\r\n", aucSoftware_Version,aucHardware_Version,aucDev_Type);
	//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);aucDev_Type

	return 0;
	
}


int OnButtonCtrlParam(char *srcmac, char *dstmac, int baudrate) 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;
	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	
	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}

	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=EXECCONTROLOD%256;//read index 0x03E9
	tempsendbuf[1]=EXECCONTROLOD/256;//read index
	tempsendbuf[2]=6;//write SUB index  baudrate  is 6
	tempsendbuf[3]=1;//read OPT baudrate

	tempsendbuf[4]=1;//len
	len = 5+1;
	if (baudrate == 9600)
	{
		tempsendbuf[5]=6;
	}
	else if (baudrate ==11520)
	{
		tempsendbuf[5]=15;
	}
	else
	{
		tempsendbuf[5]=15;
	}
	unsigned short cmd = 0x3800|WRITEFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, EXECCONTROLOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(WRITEFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf("invalid command\r");
		return -2;
	}
	else if ( receivelen <=0)
	{
		printf("no response\r");
		return -3;
	}

	return 0;
	
}

int OnButtonBaudrateSet(char *srcmac, char *dstmac, int baudrate) 
{

	char ret = 0;

	int nLen = 1;
	int dwWrittenCount = 0;
	int dwReadCount = 0;
	int trycount = 0;
	char sendbuf[500];
	char tempsendbuf[500];

	//发送0x7f
	int receivelen = 0;
	char *str = NULL;
	
	
	unsigned char crc = 0;
	char valuelen = 0;
	int len = 0;
//	unsigned char tempbuf[500];
	int templen=0;
	char responsebuf[500];
	char responsebufhex[1024];
	memset(tempsendbuf, 0, sizeof(tempsendbuf));
	memset(sendbuf, 0, sizeof(sendbuf));
	

	char srchexmac[32];
	char dsthexmac[32];
	char m_send_data[1024];
	char m_receive_data[1024];
	
	if (strlen(srcmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	if (strlen(dstmac)!=16)
	{
		//MessageBox("MAC地址必须等于16字节!");
		return -1;
	}
	GetHexData(srcmac, srchexmac);
	GetHexData(dstmac, dsthexmac);
	int i;
	for (i=0; i<4; i++)
	{
		srchexmac[i] =srchexmac[7-i];
		dsthexmac[i] =dsthexmac[7-i];
	}


	tempsendbuf[0]=EXECCONTROLOD%256;//read index 0x03E9
	tempsendbuf[1]=EXECCONTROLOD/256;//read index
	tempsendbuf[2]=6;//write SUB index  baudrate  is 6
	tempsendbuf[3]=1;//read OPT baudrate

	tempsendbuf[4]=1;//len
	len = 5+1;
	if (baudrate == 9600)
	{
		tempsendbuf[5]=6;
	}
	else if (baudrate == 115200)
	{
		tempsendbuf[5]=15;
	}
	else
	{
		tempsendbuf[5]=15;
	}
	unsigned short cmd = 0x3800|WRITEFLAG;
	int datalen = packetZigbeedata(cmd, sendbuf, tempsendbuf, len, dsthexmac,srchexmac);
	if(datalen>0)
	{
		ret=GetHexStr(m_send_data, sendbuf,datalen);
	
		receivelen = SendCOMANDWaitforRESP(sendbuf, datalen, EXECCONTROLOD,responsebuf);
		if(receivelen <= 0)
		{	
			//GetDlgItem(IDC_BUTTON_SET_MANAGEPARAM)->EnableWindow(TRUE);

		//	return;
		}
		else
		{

			ret=GetHexStr(m_receive_data, responsebuf,receivelen );

		}
	}


	char *p = responsebuf+sizeof(EMBERCMDPACKET);
	if ( receivelen >0 &&(WRITEFLAG != ((EMBERCMDPACKET*)responsebuf)->clusterid[0]))
	{
		printf("invalid command\r");
		return -2;
	}
	else if ( receivelen <=0)
	{
		printf("no response\r");
		return -3;
	}

	return 0;
	
}
