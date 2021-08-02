#include "func.h"

#define SERV_PORT 4507

pthread_mutex_t mutex;
pthread_cond_t cond;

void *my_write(void *arg)
{
    char buf[BUFSIZE];
    int cfd = *(int *)arg;

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        scanf("%s", buf);
        write(cfd, buf, BUFSIZE);
        if(strcmp(buf, "-send_file") == 0)
        {
            printf("lock_on\n");
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_unlock(&mutex);
            printf("lock_off\n");
        }
    }

    return NULL;
}

void *my_read(void *arg)
{
    int len;
    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char file_path[BUFSIZE];
    char *file_name;

    int cfd = *(int *)arg;

    struct stat buffer;


    while(1)
    {
        memset(buf, 0, sizeof(buf));
        len = read(cfd, buf, sizeof(buf));
        printf("%s\n", buf);

        if(strcmp(buf, "-send_file") == 0)  //说明需要发送文件
        {
            while(1)
            {
                strcpy(temp, "---请输入完整的路径名(q to quit):");
                printf("%s\n", temp);
                scanf("%s", file_path);
                if(strcmp(file_path, "q") == 0)
                {
                    strcpy(temp, "---取消发送文件---\n");
                    printf("%s\n", temp);

                    break;
                }
                //判断输入是否正确
                //如果目标文件或目录不存在就会报错
                if(stat(file_path, &buffer) == -1)
                {
                    printf("---非法的路径名---\n");
                    continue;
                }
                //排除掉输入目录的情况
                if(file_path[strlen(file_path) - 1] == '/')
                {
                    strcpy(temp, "---输入的是一个目录\n");
                    printf("%s\n", temp);

                    continue;
                }
                //执行到这里说明输入正确的文件的路径名

                printf("---请输入\"-start_send\"开始传送文件:\n");
                scanf("%s", buf);
                Write(cfd, buf);

                //获取文件长度并发送给server的套接字
                sprintf(buf, "%d", buffer.st_size);
                Write(cfd, buf);
                //获取文件名并发送给server的套接字
                file_name = basename(file_path);
                strcpy(buf, file_name);
                Write(cfd, buf);
                
                //开始向服务器的文件缓冲区发送文件
                printf("file_path = %s\n", file_path);
                int fp = open(file_path, O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR);

                printf("---开始传送文件:%s---\n", buf);
                sendfile(cfd, fp, 0, buffer.st_size);
                printf("---文件<%s>传送成功---\n", buf);

                close(fp);

                break;
            }
            pthread_cond_signal(&cond);
        }
        else if(strcmp(buf, "-recv_file") == 0)
        {
            while(1)
            {
                //读取服务器发来的文件长度
                Read(cfd, buf, sizeof(buf), __LINE__);
                int len = atoi(buf);
                //读取服务器发来的文件名称
                Read(cfd, buf, sizeof(buf), __LINE__);
                sprintf(temp, "/home/crushbb/Desktop/%s", buf);
                file_name = basename(temp);
                //创建文件
                FILE *fp = fopen(temp, "wb");
                if (fp == NULL) 
                {
                    perror("Can't open file");
                    exit(1);
                }
                
                //把数据写入文件
                printf("------开始接收文件<%s>------\n", file_name);
                Read(cfd, buf, len, __LINE__);
                fwrite(buf, sizeof(char), len, fp);
                memset(buf, 0, sizeof(buf));
                printf("------文件<%s>接收成功------\n", file_name);

                fclose(fp);

                break;
            }
        }
    }

    return NULL;
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

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cfd == -1)
        my_err("socket error", __LINE__);

    if(connect(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        my_err("connect error", __LINE__);
    

    pthread_t wthid, rthid;

    if(pthread_create(&wthid, NULL, my_write, (void *)&cfd) == -1)
        my_err("pthread_create error", __LINE__);
    if(pthread_create(&rthid, NULL, my_read, (void *)&cfd) == -1)
        my_err("pthread_create error", __LINE__);


    pthread_join(wthid, NULL);
    pthread_join(rthid, NULL);

    close(cfd);

    return 0;
}