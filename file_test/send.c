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
    if((n = write(fd, buf, BUFSIZ)) == -1)
    {   
        my_err("write error", __LINE__);
    }
}

int main(int argc, char **argv)
{
    //检查参数个数
    if(argc != 5)
    {
        printf("Usage: [-p] [serv_port] [-a] [serve_address]\n");
        exit(1);
    }
    int i;
    int serv_port;
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    // serv_addr.sin_port = htons(SERV_PORT);
    // inet_aton("127.0.0.1", &serv_addr.sin_addr);
    //从命令行的输入获取服务器端的端口与地址
    for(i=1; i<argc; i++)
    {
        if(strcmp(argv[i], "-p") == 0)
        {
            serv_port = atoi(argv[i+1]);
            if(serv_port < 0 || serv_port > 65535)
            {
                printf("Invalid serv_addr.sin_port\n");
                exit(1);
            }
            else
            {
                serv_addr.sin_port = htons(serv_port);
            }
            continue;
        }

        if(strcmp(argv[i], "-a") == 0)
        {
            if(inet_aton(argv[i+1], &serv_addr.sin_addr) == 0)
            {
                printf("Invalid server ip address\n");
                exit(1);
            }
            continue;
        }
    }

    int cfd;
    char buf[BUFSIZE];
    int len;

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cfd == -1)
        my_err("socket error", __LINE__);

    if(connect(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        my_err("connect error", __LINE__);

    char file_path[BUFSIZE];
    char *file_name;
    char temp[BUFSIZE];
    struct stat buffer;

    printf("输入完整的文件名:");
    scanf("%s", file_path);
    //判断输入是否正确
    //如果目标文件或目录不存在就会报错
    if(stat(file_path, &buffer) == -1)
    {
        printf("---非法的路径名---\n");
        return 0;
    }
    file_name = basename(file_path);
    struct node
    {
        int len;
        char name[100];
    }node;
    node.len = buffer.st_size;
    strcpy(node.name, file_name);
    
    memcpy(temp, &node, sizeof(node));
    write(cfd, temp, sizeof(temp));
    
    //开始向服务器的文件缓冲区发送文件
    int fp = open(file_path, O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR);

    printf("---开始传送文件:%s---\n", node.name);
    sendfile(cfd, fp, 0, buffer.st_size);
    printf("---文件<%s>传送成功---\n", node.name);

    close(fp);
    // close(cfd);

    return 0;
}