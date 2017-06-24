#include "tcp_rs232.h"

int get_loacl_time (char *local_time)//��ȡ���ص�ʱ��
{
	int ret = 0;
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	struct tm *p;
	time_t timep; //�ѵ�ǰʱ���timep

	if( time(&timep) < 0 )
	{
	#ifdef DEBUG
		perror("time");
	#endif
		ret = -1;
        }
	else
	{
		p = localtime(&timep); // Get the loacl system time
		sprintf(local_time, "%d-%d-%d %s %02d:%02d:%02d : ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
	}
	return ret;
}

void log_to_file (const char *str1, char *str2, char *str3)
{
	int ret = 0, rcv = 0;
	struct stat log_file;
	FILE *fd;
	char local_time[32];

    #ifdef DEBUG_log
	if (str1)
	{
		printf("%s", str1);
	}
	if (str2)
	{
		printf("%s", str2);
	}
	if (str3)
	{
		printf("%s", str3);
	}
	printf("\n");
    #endif

	/** open log file */
	rcv = stat(LOG_FILE_PATH, &log_file);//ͨ���ļ�����ȡ�ļ���Ϣ������������ָ�Ľṹ��stat��
	if (rcv < 0)// ִ�гɹ��򷵻�0��ʧ�ܷ���-1
	{
	    #ifdef DEBUG
		perror("stat");
	    #endif
		fd = fopen(LOG_FILE_PATH, "a");
	}
	else
	{
		if (log_file.st_size > 102400)//�������100kb
		{
			fd = fopen(LOG_FILE_PATH, "w");//���ļ���ɾ�����½�һ���ļ�������д����
		}
		else
		{
			fd = fopen(LOG_FILE_PATH, "a");//���ı��ļ�β׷��
		}
	}

	if(fd == NULL)
	{
	    #ifdef DEBUG
		perror("fopen");
	    #endif
		return;
	}
	else
	{
		memset(local_time, '\0', sizeof(local_time));
		if (get_loacl_time(local_time) == 0)//��ȡ��ǰʱ������ڲ�ת��Ϊ����ʱ�����local_time������
		{
			fputs(local_time, fd);//��ʱ�������fd��ָ���ļ���,�ļ���λ��ָ����Զ�����
		}
		fputs("  ", fd);//���ļ���ĩβ��ӿո�

		if (str1)
 		{
			fputs(str1, fd);
		}
		fputc('\t', fd);//����ȷд��һ���ַ���һ���ֽڵ����ݺ��ļ��ڲ�дָ����Զ�����һ���ֽڵ�λ��
		fputc('\t', fd);

		if (str2)
		{
			fputs(str2, fd);
		}
		fputc('\t', fd);
		fputc('\t', fd);

		if (str3)
		{
			fputs(str3, fd);
		}
		fputc('\n', fd);
		fclose(fd);
	}
	return;
}

typedef void (*sighandler_t)(int);
int pox_system(const char *cmd_line)
{
	int ret = 0;
	sighandler_t old_handler;
	old_handler = signal(SIGCHLD, SIG_DFL);
	ret = system(cmd_line);
	signal(SIGCHLD, old_handler);
	return ret;
}

void HexToStr (const uint8 *digest,uint8 (*buff)[HEXLEN],uint32 len)
{
   int i;
   for(i = 0; i < len; i++)
   {
     if(digest[i] >= 0 && digest[i] <= 15)
        sprintf(buff[i], "0%X", digest[i]);
     else
        sprintf(buff[i], "%X", digest[i]);                                                                          
   }
}
uint8 StrToHex (char hight_ch,char low_ch)
{
    uint8 f1 = 0;
   if(hight_ch  >= '0' && hight_ch <= '9')
	f1 = (hight_ch - '0') * 16;
   else if(hight_ch >= 'A' && hight_ch <= 'F')
	f1 = (hight_ch - 'A' + 10) * 16;
   uint8 t1 = 0;
   if(low_ch >= '0' && low_ch <= '9')
      t1 = (low_ch - '0');
   else if(low_ch >= 'A' && low_ch <= 'F')
      t1 = (low_ch - 'A' + 10);
  return f1 + t1; 
}
