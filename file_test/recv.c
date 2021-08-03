#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <libgen.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include "time.h"

#define SERV_PORT 4507
#define BUFSIZE 1024
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
    int n;
    if((n = write(fd, buf, BUFSIZE)) == -1)
    {   
        my_err("write error", __LINE__);
    }
}

int main()
{
    int lfd, cfd;
    int clit_addr_len;
    struct sockaddr_in serv_addr, clit_addr;
    
    //初始化服务器
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1)
        my_err("socket error", __LINE__);

    int optval = 1;
    if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(int)) < 0)
        my_err("setsockopt error", __LINE__);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        my_err("bind error", __LINE__);

    if(listen(lfd, 128) == -1)
        my_err("listen error", __LINE__);

    clit_addr_len = sizeof(clit_addr);
    cfd = accept(lfd, (struct sockaddr *)&clit_addr, &clit_addr_len);
    if(cfd == -1)
        my_err("accept error", __LINE__);

    char buf[BUFSIZ];
    char temp[BUFSIZ];
    char name[100];





    //获取用户端传来的文件长度
    unsigned long long len;
    Read(cfd, buf, sizeof(buf), __LINE__);
    len = atoi(buf);
    // printf("len = %d\n", len);
    int n;
    //获取客户端传来的文件名于buf中
    Read(cfd, buf, sizeof(buf), __LINE__);
    // printf("file_name = %s\n", buf);
    sprintf(temp, "./file_recv/%s", buf);
    //创建文件
    FILE *fp = fopen(temp, "wb");
    if (fp == NULL) 
    {
        perror("Can't open file");
        exit(1);
    }
    
    //把数据写入文件
    printf("Start receive file: %s from %s\n", temp, inet_ntoa(clit_addr.sin_addr));


    while((n = Read(cfd, buf, BUFSIZ, __LINE__)) > 0)
    {
        printf("n = %d\n", n);
        fwrite(buf, sizeof(char), n, fp);
    }

    Read(cfd, buf, len, __LINE__);
    fwrite(buf, sizeof(char), len, fp);
    
    
    puts("Receive Success");

    // 关闭文件
    fclose(fp);    



    close(lfd);
    close(cfd);

    return 0;
}