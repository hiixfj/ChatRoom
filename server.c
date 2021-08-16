#include "func.h"

#define SERV_PORT 4507
//#define MAXEVE 1024

pthread_mutex_t mutex;

int num_birth;
int clients[1000];
int birth;

int main()
{
    int lfd;
    int epfd;
    struct epoll_event tep, ep[MAXEVE];

    int cfd;
    struct sockaddr_in serv_addr, clit_addr;
    int clit_addr_len;
    int ret;
    int i, j = 0;
    int n;
    char buf[BUFSIZE];
    int flag;

    MYSQL mysql;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char *query_str = NULL;
    int rc, fields;
    int rows;

    char temp[BUFSIZE];

    pthread_t thid;

    pthread_mutex_init(&mutex, NULL);

    //初始化数据库
    if (NULL == mysql_init(&mysql))
    {
        printf("mysql_init(): %s\n", mysql_error(&mysql));
        return -1;
    }
    if (NULL == mysql_real_connect(&mysql,
                                   "127.0.0.1",
                                   "root",
                                   "xjmwsb1234",
                                   "testdb",
                                   0,
                                   NULL,
                                   0))
    {
        printf("mysql_real_connect(): %s\n", mysql_error(&mysql));
        return -1;
    }
    printf("1. Connected MySQL successful! \n");

    /*
    *
    *   启动服务器之后，先检测mysql指定的库中有没有必备表
    *     
    */
    int a[3];
    memset(a, 0, sizeof(a));
    query_str = "show tables;";
    MY_real_query(&mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&mysql);
    if (res == NULL)
        my_err("mysql_store_result error", __LINE__);
    while (row = mysql_fetch_row(res))
    {
        if (strcmp(row[0], "UserData") == 0)
        {
            a[0] = 1;
        }
        else if (strcmp(row[0], "HisData") == 0)
        {
            a[1] = 1;
        }
        else if (strcmp(row[0], "OffLineMes") == 0)
        {
            a[2] = 1;
        }
    }
    for (int i = 0; i < 3; i++)
    {
        if (a[i] == 0)
        {
            switch (i)
            {
                case 0:
                    query_str =
                        "create table UserData \
                (username varchar(20), password varchar(20), nickname varchar(20), mibao varchar(20), \
                num double, status double, cfd double, newsnum double)";
                    MY_real_query(&mysql, query_str, strlen(query_str), __LINE__);
                    break;

                case 1:
                    query_str =
                        "create table HisData \
                (time varchar(100), inuser varchar(20), touser varchar(20), infor varchar(200))";
                    MY_real_query(&mysql, query_str, strlen(query_str), __LINE__);
                    break;

                case 2:
                    query_str =
                        "create table OffLineMes \
                (time varchar(100), inuser varchar(20), touser varchar(20), infor varchar(200), type double)";
                    MY_real_query(&mysql, query_str, strlen(query_str), __LINE__);
                    break;

                default:
                    break;
            }
        }
    }

    query_str = "select * from UserData";
    rc = mysql_real_query(&mysql, query_str, strlen(query_str));
    if (rc != 0)
        my_err("mysql_real_query error", __LINE__);
    res = mysql_store_result(&mysql);
    if (res == NULL)
        my_err("mysql_store_query", __LINE__);
    rows = mysql_num_rows(res);
    fields = mysql_num_fields(res);
    //每次启动服务器时，检索UserData中num的最大值并赋值给num_birth
    //并把每个用户的status置为0（非在线状态）
    while (row = mysql_fetch_row(res))
    {
        for (i = 0; i < fields; i++)
        {
            num_birth = atoi(row[4]);
        }
    }
    num_birth += 1;
    query_str = "update UserData set status = 0 where status = \"1\"";
    MY_real_query(&mysql, query_str, strlen(query_str), __LINE__);
    query_str = "update UserData set status = 1 where username = \"root\"";
    MY_real_query(&mysql, query_str, strlen(query_str), __LINE__);

    //初始化服务器
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
        my_err("socket error", __LINE__);

    int optval = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(int)) < 0)
        my_err("setsockopt error", __LINE__);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        my_err("bind error", __LINE__);

    if (listen(lfd, 128) == -1)
        my_err("listen error", __LINE__);

    epfd = epoll_create(MAXEVE);
    tep.data.fd = lfd;
    tep.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &tep);

    printf("---initializing...\n");
    printf("---loading finished\n");

    struct cfd_mysql cm;
    void *retval;
    memset(clients, 0, sizeof(clients));
    cm.tocfd = epfd;

    signal(SIGPIPE, SIG_IGN);

    while (1)
    {
        //等待客户端的连接请求
        ret = epoll_wait(epfd, ep, MAXEVE, -1);
        for (i = 0; i < ret; i++)
        {
            if (ep[i].data.fd == lfd)  //lfd满足读事件，有新的客户端发起连接请求
            {
                clit_addr_len = sizeof(clit_addr);
                cfd = accept(lfd, (struct sockaddr *)&clit_addr, &clit_addr_len);
                if (cfd == -1)
                    my_err("accept error", __LINE__);

                tep.data.fd = cfd;
                tep.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &tep);

                printf("ip %s is connect\n", inet_ntoa(clit_addr.sin_addr));
                strcpy(temp, "Welcome to my_server!\n");
                Write(cfd, temp);
                strcpy(temp, "输入任意建进入欢迎界面\n");
                Write(cfd, temp);
            }
            else  //cfd们满足读事件，有客户端数据写来
            {
                //判断数组clients中有没有此用户的套接字
                j = 0;
                flag = 0;
                while (j < 1000)
                {
                    if (clients[j] == ep[i].data.fd)
                    {
                        flag = 1;
                        break;
                    }
                    j++;
                }
                if (flag == 1)
                {
                    printf("%d : cfd = %d\n", __LINE__, ep[i].data.fd);
                    break;
                }
                //读套接字
                n = read(ep[i].data.fd, buf, sizeof(buf));
                if (n == 0)
                {
                    for (j = 0; j < birth; j++)
                    {
                        if (clients[j] == ep[i].data.fd)
                        {
                            clients[j] = 0;
                        }
                    }
                    close(ep[i].data.fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep[i].data.fd, NULL);
                }
                else if (n > 0)  //接受到客户端的消息，就转发给子线程serv_new_client()处理
                {
                    //判断数组clients中有没有此用户的套接字
                    j = 0;
                    flag = 0;
                    while (j < 1000)
                    {
                        if (clients[j] == ep[i].data.fd)
                        {
                            flag = 1;
                            break;
                        }
                        j++;
                    }
                    printf("flag = %d\n", flag);
                    printf("cfd = %d\n", ep[i].data.fd);
                    if (flag == 0)  //flag = 0 证明该客户端还未创建属于它的子线程
                    {
                        cm.cfd = ep[i].data.fd;
                        cm.mysql = mysql;
                        cm.clit_addr = clit_addr;

                        clients[birth] = cfd;
                        birth++;

                        if (pthread_create(&thid, NULL, serv_new_client, (void *)&cm) == -1)
                        {
                            my_err("pthread_create error", __LINE__);
                        }
                    }
                }
                // else
                // {
                //     my_err("read error", __LINE__);
                // }
            }
        }
    }

    for (i = 0; i < ret; i++)
    {
        close(ep[i].data.fd);
    }

    return 0;
}