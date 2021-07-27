#include "func.h"

#define SERV_PORT 4507

void *my_write(void *arg)
{
    char buf[BUFSIZ];
    int cfd = *(int *)arg;

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        scanf("%s", buf);
        write(cfd, buf, BUFSIZ);
    }

    return NULL;
}

void *my_read(void *arg)
{
    int len;
    char buf[BUFSIZ];
    int cfd = *(int *)arg;

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        len = read(cfd, buf, sizeof(buf));
        printf("%s\n", buf);
    }

    return NULL;
}



int main()
{
    int cfd;
    struct sockaddr_in serv_addr;
    char buf[BUFSIZ];
    int len;

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cfd == -1)
        my_err("socket error", __LINE__);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
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