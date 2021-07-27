#include "func.h"

extern int num_birth;
extern pthread_mutex_t mutex;

void *func_Group_options(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    char buf[BUFSIZ];
    char temp[BUFSIZ];
    char query_str[BUFSIZ];

    while(1)
    {
        memset(temp, 0, sizeof(temp));
        strcpy(temp, "------群选项------\n");
        Write(cm.cfd, temp);

        strcpy(temp, "[1] 创建群\n[2] 解散群\n[3] 申请加群\n[q] 返回");
        Write(cm.cfd, temp);

        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "1") == 0)
        {
            if(pthread_create(&thid, NULL, Group_create, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "2") == 0)
        {
            if(pthread_create(&thid, NULL, Group_disband, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "3") == 0)
        {

        }
        else if(strcmp(buf, "q") == 0)
        {
            break;
        }
        else
        {
            strcpy(temp, "---没有此选项，请输入1/2/3/q:");
            Write(cm.cfd, temp);
            continue;
        }
    }


    pthread_exit(0);
}


void *Group_create(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag = 0;

    char buf[BUFSIZ];
    char temp[BUFSIZ];
    char query_str[BUFSIZ];

    memset(temp, 0, sizeof(temp));
    strcpy(temp, "---创建群---\n");
    Write(cm.cfd, temp);

    while(1)
    {
        strcpy(temp, "---请输入群昵称(q to quit):");
        Write(cm.cfd, temp);

        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            break;
        }
        //查询UserData是否重复命名
        flag = 0;
        strcpy(query_str, "select * from UserData");
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res = mysql_store_result(&cm.mysql);
        if(res == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row = mysql_fetch_row(res))
        {
            if(strcmp(row[0], buf) == 0)
            {
                flag = 1;   //有重复命名
                break;
            }
        }
        if(flag == 1)
        {
            strcpy(temp, "---该名已存在");
            continue;
        }

        //把该群名以及群主名加入UserData中，并把status设置为2
        sprintf(query_str, "insert into UserData values\
        (\"%s\", \"%s\", NULL, NULL, \"%d\", 2, NULL, 0)", \
        buf,     cm.username,      num_birth);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

        //更新num_birth的值
        pthread_mutex_lock(&mutex);
            num_birth++;
        pthread_mutex_unlock(&mutex);

        //为这个群创建一个表，用来储存群成员
        sprintf(query_str, "create table %s(username varchar(20), type double)", buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

        //将群主设置为自己
        sprintf(query_str, "insert into %s values\
                    (\"%s\", \"2\")", cm.username);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

        break;
    }

    pthread_exit(0);
}

void *Group_disband(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    int flag;
    int master_flag;

    char buf[BUFSIZ];
    char temp[BUFSIZ];
    char query_str[BUFSIZ];

    memset(query_str, 0, sizeof(query_str));
    memset(buf, 0, sizeof(buf));
    memset(temp, 0, sizeof(temp));

    strcpy(temp, "------解散群------\n");
    Write(cm.cfd, temp);

    while(1)
    {
        strcpy(temp, "---输入群名以解散群(q to quit):");
        Write(cm.cfd, temp);

        //列出自己是群主的群名
        sprintf(query_str, "select * from UserData \
        where password = \"%s\" and status = \"2\"", \
                        cm.username);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res = mysql_store_result(&cm.mysql);
        if(res == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row = mysql_fetch_row(res))
        {
            sprintf(temp, "---<%s>\n", row[0]);
            Write(cm.cfd, temp);
        }

        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            break;
        }
        flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
        if(flag == 1)
        {
            strcpy(temp, "---不存在这个用户/群\n");
            Write(cm.cfd, temp);
            continue;
        }
        sprintf(query_str, "select status from UserData\
        where username = \"%s\"", buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res2 = mysql_store_result(&cm.mysql);
        if(res2 == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row2 = mysql_fetch_row(res2))
        {
            if(atoi(row2[0]) == 2)
            {
                master_flag = 1;    //这个名称是群
            }
            else
            {
                master_flag = 0;    //这个名称不是群，是个人用户
            }
        }
        if(flag == 0 && master_flag == 1)   //输入正确
        {
            sprintf(temp, "你确定要解散群<%s>？<y/n>\n", buf);
            Write(cm.cfd, temp);
            
        }
    }

    pthread_exit(0);
}
