#include "tcp_rs232.h"

int get_loacl_time (char *local_time)//获取当地的时间
{
	int ret = 0;
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	struct tm *p;
	time_t timep; //把当前时间给timep

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
	rcv = stat(LOG_FILE_PATH, &log_file);//通过文件名获取文件信息，并保存在所指的结构体stat中
	if (rcv < 0)// 执行成功则返回0，失败返回-1
	{
	    #ifdef DEBUG
		perror("stat");
	    #endif
		fd = fopen(LOG_FILE_PATH, "a");
	}
	else
	{
		if (log_file.st_size > 102400)//如果大于100kb
		{
			fd = fopen(LOG_FILE_PATH, "w");//有文件就删除重新建一个文件往里面写数据
		}
		else
		{
			fd = fopen(LOG_FILE_PATH, "a");//向文本文件尾追加
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
		if (get_loacl_time(local_time) == 0)//获取当前时间和日期并转换为本地时间存入local_time数组中
		{
			fputs(local_time, fd);//将时间输出到fd所指的文件中,文件的位置指针会自动后移
		}
		fputs("  ", fd);//在文件的末尾添加空格

		if (str1)
 		{
			fputs(str1, fd);
		}
		fputc('\t', fd);//当正确写入一个字符或一个字节的数据后，文件内部写指针会自动后移一个字节的位置
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
