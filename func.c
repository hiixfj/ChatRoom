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
    // printf("readn = %d\n", n);
    // printf("readbuf = %s\n", buf);
    if(n < 0)
    {
        my_err("read error", line);
    }
    return n;
}

void Write(int fd, const char *buf)
{  
    int n;
    if((n = write(fd, buf, strlen(buf))) == -1)
    {   
        my_err("write error", __LINE__);
    }
    // printf("writen = %d\n", n);
    // printf("writebuf = %s\n", buf);
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

void Sendfile(FILE *fp, int sockfd) 
{
    int n; //每次读取数据数量
    char sendline[MAX_LINE] = {0}; //暂存每次读取的数据
    while ((n = fread(sendline, sizeof(char), MAX_LINE, fp)) > 0) 
    {
        if (n != MAX_LINE && ferror(fp)) //读取出错并且没有到达文件结尾
        {
            perror("Read File Error");
            exit(1);
        }
        
        //将读取的数据发送到TCP发送缓冲区
        if (send(sockfd, sendline, n, 0) == -1)
        {
            perror("Can't send file");
            exit(1);
        }
        memset(sendline, 0, MAX_LINE); //清空暂存字符串
    }
}

void Writefile(int sockfd, FILE *fp)
{
    ssize_t n; //每次接受数据数量
    char buff[MAX_LINE] = {0}; //数据缓存
    while ((n = recv(sockfd, buff, MAX_LINE, 0)) > 0) 
    {
        if (n == -1)
        {
            perror("Receive File Error");
            exit(1);
        }
        
        //将接受的数据写入文件
        if (fwrite(buff, sizeof(char), n, fp) != n)
        {
            perror("Write File Error");
            exit(1);
        }
        memset(buff, 0, MAX_LINE); //清空缓存
    }
}