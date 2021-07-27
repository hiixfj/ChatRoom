#include "func.h"

void my_err(const char *str, const int line)
{
    fprintf(stderr, "%d : %s : %s", line, str, strerror(errno));
    exit(1);
}

int Read(int fd, char *buf, size_t count, int line)
{
    int n = 0;
    memset(buf, 0, count);

    n = read(fd, buf, count);
    if(n < 0)
    {
        my_err("read error", line);
    }
    return n;
}

void Write(int fd, const char *buf)
{  
    if(write(fd, buf, BUFSIZ) == -1)
    {   
        my_err("write error", __LINE__);
    }
}

char *get_time(char *now_time)
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	strcpy(now_time, asctime (timeinfo));
	now_time[strlen(now_time)-1] = '\0';

    return now_time;
}

int get_userinfo(char *buf, int len)
{
    int i = 0;
    char c;
    
    if(buf == NULL)
        my_err("buf is NULL", __LINE__);

    while(((c = getchar()) != '\n') && (c != EOF) && (i < len - 2))
    {
        buf[i] = c;
        i++;
    }
    buf[i] = '\n';
    i++;
    buf[i] = '\0';

    return 0;
}