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

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];

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
            if(pthread_create(&thid, NULL, Group_apply, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
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

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];

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
                    (\"%s\", \"2\")", buf, cm.username);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

        //将这个群加入到自己的好友列表中，并把type设置为4
        sprintf(query_str, "insert into %s values \
        (\"%s\", \"4\")", cm.username, buf);
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

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];
    char now_time[BUFSIZE];

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
            strcpy(group_name, buf);
            Write(cm.cfd, temp);
            Read(cm.cfd, buf, sizeof(buf), __LINE__);
            if(strcmp(buf, "n") == 0)
            {
                strcpy(temp, "---解散群取消\n");
                Write(cm.cfd, temp);
                continue;
            }
            else if(strcmp(buf, "y") == 0)
            {
                //确认解散群
                //<群主>-<群>s:该群已被群主解散
                //向群内成员广播,type为(3)：群只读类消息
                //
                //广播
                //
                strcpy(cm.tousername, group_name);
                strcpy(temp, "该群已被群主解散");
                Group_broadcast((void *)&cm, temp, 3);


                // strcpy(temp, "该群已被群主解散");
                // sprintf(query_str, "insert into OffLineMes values \
                // (\"%s\", \"%s\", \"%s\", \"%s\", \"3\")", \
                // get_time(now_time), cm.username, group_name, temp);
                // MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                //将该群在所有群成员的好友列表中移除
                sprintf(query_str, "select * from %s", group_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    sprintf(query_str, "delete from %s where username = \"%s\"", \
                                                    row[0],            group_name);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                }

                //将群在UserData中移除
                //将群在tables中移除
                sprintf(query_str, "delete from UserData \
                where username = \"%s\"", group_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                
                sprintf(query_str, "drop table %s", group_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                sprintf(temp, "---已成功解散群<%s>", group_name);
                Write(cm.cfd, temp);
            }
            else 
            {
                continue;
            }
        }
    }

    pthread_exit(0);
}

void *Group_apply(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    int flag;
    int confirm_flag;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];
    char now_time[BUFSIZE];

    strcpy(temp, "------申请加群------\n");
    Write(cm.cfd, temp);

    while(1)
    {
        confirm_flag = 0;

        strcpy(temp, "---输入群名以发送加群申请(q to quit):");
        Write(cm.cfd, temp);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);

        if(strcmp(buf, "q") == 0)
        {
            break;
        }
        //先判断该名是否存在于UserData中
        flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
        if(flag == 1)
        {
            strcpy(temp, "---不存在此名\n");
            Write(cm.cfd, temp);
            continue;
        }
        strcpy(group_name, buf);
        //然后判断该名的status是否为2
        sprintf(query_str, "select status from UserData \
        where username = \"%s\"", group_name);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res = mysql_store_result(&cm.mysql);
        if(res == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row = mysql_fetch_row(res))
        {
            if(atoi(row[0]) == 2)       
            {
                confirm_flag = 1;
            }
        }
        if(confirm_flag == 0)   //输入错误，输入为用户名
        {
            continue;
        }

        //执行到这里说明输入正确
        //开始发送加群申请到OffLineMes
        sprintf(temp, "%s：请求加入群聊：%s", cm.username, group_name);
        sprintf(query_str, "insert into OffLineMes values \
        (\"%s\", \"%s\", \"%s\", \"%s\", \"5\")", \
        get_time(now_time), cm.username, group_name, temp);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

        strcpy(temp, "---加群请求发送成功，等待管理员审核\n");
        Write(cm.cfd, temp);
        
        //申请加群流程走完，退出Group_apply线程
        break;
    }

    pthread_exit(0);
}

void Group_broadcast(void *arg, char *q, int type)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    int flag;
    int group_flag;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];
    char now_time[BUFSIZE];

    strcpy(temp, q);
    strcpy(group_name, cm.tousername);

    sprintf(query_str, "select * from %s", group_name);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    //广播消息---把消息发给所有群成员
    while(row = mysql_fetch_row(res))
    {
        sprintf(query_str, "select * from UserData \
                    where username = \"%s\"", row[0]);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res2 = mysql_store_result(&cm.mysql);
        if(res2 == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row2 = mysql_fetch_row(res2))
        {
            if(strcmp(row2[0], cm.username) != 0)
            {
                if(atoi(row2[5]) == 1)      //群成员在线的话，直接向对方的套接字发送内容
                {
                    //获取群成员row2的套接字
                    cm.tocfd = atoi(row2[6]);
                    //发送消息
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "<%s>-<%s>-<%s>:%s", \
                        get_time(now_time), group_name, cm.username, temp);
                    Write(cm.tocfd, buf);
                }
                else if(atoi(row2[5]) == 0) //该群成员不在线的话，把消息添加到OffLineMes中
                {
                    sprintf(query_str, "insert into OffLineMes values \
                    (\"%s\", \"%s\", \"%s\", \"%s\", \"%d\")", \
                    get_time(now_time), cm.username, row2[0], temp, type);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                }
            }
        } 
    }
    //把消息存入HisData中
    sprintf(query_str, "insert into HisData values \
    (\"%s\", \"%s\", \"%s\", \"%s\")", \
    now_time, cm.username, group_name, temp);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
}

void *func_group_list(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    int group_flag;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];

    while(1)
    {
        group_flag = 0;

        strcpy(temp, "---输入群名称进入群聊(q to quit):");
        Write(cm.cfd, temp);
        //打印出自己加入的群聊
        sprintf(query_str, "select * from %s \
        where num = \"2\" or num = \"3\" or num = \"4\"", \
                                        cm.username);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res = mysql_store_result(&cm.mysql);
        if(res == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row = mysql_fetch_row(res))
        {
            sprintf(temp, "---<%s>", row[0]);
            Write(cm.cfd, temp);
        }

        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            break;
        }
        
        //判断输入是否正确
        flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
        if(flag == 0)
        {
            sprintf(query_str, "select status from UserData \
            where username = \"%s\"", buf);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }
            while(row = mysql_fetch_row(res))
            {
                if(atoi(row[0]) == 2)
                {
                    group_flag = 1;
                }
            }
        }

        if(group_flag == 0)
        {
            strcpy(temp, "---不存在此群\n");
            Write(cm.cfd, temp);
            continue;
        }

        //执行到这里说明输入正确的群名了
        strcpy(group_name, buf);
        strcpy(cm.tousername, buf);
        //进入群聊界面
        if(pthread_create(&thid, NULL, Group_chat, (void *)&cm) == -1)
        {
            my_err("pthread_create error", __LINE__);
        }
        pthread_join(thid, NULL);
        continue;
    }

    pthread_exit(0);
}

void Group_h(void *arg, int identity)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    char temp[BUFSIZE];

    sprintf(temp, "------%s(group)------\n", cm.tousername);
    Write(cm.cfd, temp);
    //打印出菜单栏
    //如果是管理员的话，打印额外一条参数---踢人
    if(identity == 1)
    {
        strcpy(temp, "------(\"-kick_member\" to make member out of the group)------\n");
        Write(cm.cfd, temp);
    }
    else if(identity == 2)      //如果是群主的话，打印额外两条参数---踢人---设置群管理员
    {
        strcpy(temp, "------(\"-kick_member\" to kick member)------\n");
        Write(cm.cfd, temp);

        strcpy(temp, "------(\"-Set_revoke_administrator\" to set or revoke administrator)------\n");
        Write(cm.cfd, temp);
    }
    //打印参数
    strcpy(temp, "------(\"quit-exit\" to quit)------\n");
    Write(cm.cfd, temp);

    strcpy(temp, "------(\"-history\" to view chat history)------\n");
    Write(cm.cfd, temp);

    strcpy(temp, "------(\"-view_group_member\" to view member)------\n");
    Write(cm.cfd, temp);

    strcpy(temp, "------(\"-exit_group_chat\" to exit group chat)------\n");
    Write(cm.cfd, temp);

    strcpy(temp, "------(\"-h\" for help)------\n");
    Write(cm.cfd, temp);

    sprintf(temp, "------%s(group)------\n", cm.tousername);
    Write(cm.cfd, temp);
}

void *Group_chat(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    int group_flag;
    int identity;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];

    strcpy(group_name, cm.tousername);


    //先将自己的未读消息打印出来
    sprintf(query_str, "select * from OffLineMes \
    where touser = \"%s\" and type = \"4\"", cm.username);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        sprintf(temp, "<%s>-<%s>-<%s>:%s", \
                row[0], row[1], group_name, row[3]);
        Write(cm.cfd, temp);
        
        //将这条消息从OffLineMes中抹去
        sprintf(query_str, "delete from OffLineMes \
        where time = \"%s\"", row[0]);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }

    //判断username在群中是什么身份
    sprintf(query_str, "select * from %s where username = \"%s\"", \
                                    group_name,         cm.username);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        identity = atoi(row[1]);
    }

    //打印帮助菜单
    Group_h((void *)&cm, identity);

    //输入消息
    while(1)
    {   
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "quit-exit") == 0)
        {
            break;
        }
        else if(strcmp(buf, "-history") == 0)
        {
            sprintf(query_str, "select * from HisData where touser = \"%s\"", \
                                                                group_name);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }
            while(row = mysql_fetch_row(res))
            {
                sprintf(temp, "<%s>-<%s>-<%s>:%s", \
                    row[0], row[2], row[1], row[3]);
                Write(cm.cfd, temp);
            }
            sprintf(temp, "------%s------\n", cm.tousername);
            Write(cm.cfd, temp);
        }
        else if(strcmp(buf, "-view_group_member") == 0)
        {
            Group_view_member((void *)&cm);
            continue;
        }
        else if(strcmp(buf, "-exit_group_chat") == 0)   //退群
        {
            cm.tocfd = identity;
            if(pthread_create(&thid, NULL, Group_exit_group_chat, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "-kick_member") == 0)       //踢人
        {
            cm.tocfd = identity;
            if(pthread_create(&thid, NULL, Group_kick_member, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "-Set_revoke_administrator") == 0)
        {
            cm.tocfd = identity;
            if(pthread_create(&thid, NULL, Group_Set_revoke_admini, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "-h") == 0)
        {
            Group_h((void *)&cm, identity);
            continue;
        }
        else    //发消息
        {
            Group_broadcast((void *)&cm, buf, 4);
        }
    }


    pthread_exit(0);
}


void Group_view_member(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;


    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    int flag;
    int status;
    int identity;

    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];

    strcpy(group_name, cm.tousername);

    sprintf(query_str, "select * from %s", group_name);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        sprintf(query_str, "select status from UserData \
        where username = \"%s\"", row[0]);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        res2 = mysql_store_result(&cm.mysql);
        if(res2 == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row2 = mysql_fetch_row(res2))
        {
            status = atoi(row2[0]);
        }
        if(status == 1)
        {
            switch (atoi(row[1]))
            {
                case 0:
                    sprintf(temp, "---<%s>---在线\n", row[0]);
                    Write(cm.cfd, temp);
                    break;
                
                case 1:
                    sprintf(temp, "---<%s>---<管理员>---在线\n", row[0]);
                    Write(cm.cfd, temp);
                    break;

                case 2:
                    sprintf(temp, "---<%s>---<群主>---在线\n", row[0]);
                    Write(cm.cfd, temp);
                    break;

                default:
                    break;
            }
        }
        else if(status == 0)
        {
            switch (atoi(row[1]))
            {
                case 0:
                    sprintf(temp, "---<%s>---离线\n", row[0]);
                    Write(cm.cfd, temp);
                    break;
                
                case 1:
                    sprintf(temp, "---<%s>---<管理员>---离线\n", row[0]);
                    Write(cm.cfd, temp);
                    break;

                case 2:
                    sprintf(temp, "---<%s>---<群主>---离线\n", row[0]);
                    Write(cm.cfd, temp);
                    break;

                default:
                    break;
            }
        }
    }
}

void *Group_exit_group_chat(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    int group_flag;
    int identity;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];
    char group_master[BUFSIZE];

    strcpy(group_name, cm.tousername);
    identity = cm.tocfd;

    while(1)
    {
        sprintf(temp, "---你确定要退出群<%s>吗?(y/n)", group_name);
        Write(cm.cfd, temp);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "n") == 0)
        {
            sprintf(temp, "---退出群<%s>请求取消---\n", group_name);
            Write(cm.cfd, temp);
            break;
        }
        else if(strcmp(buf, "y") == 0)
        {
            if(identity == 2)       //群主退群需要特殊步骤
            {
                strcpy(temp, "---你是本群的群主，在退群之前需要将群主转移---\n");
                Write(cm.cfd, temp);

                while(1)
                {
                    group_flag = 0;
                    //打印出群成员列表
                    Group_view_member((void *)&cm);
                    strcpy(temp, "---请输入你要转移的用户名以转交群主(q to quit):\n");
                    Write(cm.cfd, temp);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    flag = mysql_repeat(&cm.mysql, group_name, buf, 1);
                    if(flag == 1)
                    {
                        strcpy(temp, "---群内不存在此用户---\n");
                        continue;
                    }
                    //判断输入的群主继承者是否是现在的群主
                    strcpy(group_master, buf);
                    sprintf(query_str, "select * from %s where username = \"%s\"", group_name, group_master);
                    res = mysql_store_result(&cm.mysql);
                    if(res == NULL)
                    {
                        my_err("mysql_store_result error", __LINE__);
                    }
                    while(row = mysql_fetch_row(res))
                    {
                        if(atoi(row[1]) == 2)
                        {
                            group_flag = 1;
                        }
                    }
                    if(group_flag == 1)
                    {
                        strcpy(temp, "---你不可以把群主转移给自己");
                        Write(cm.cfd, temp);
                        continue;
                    }

                    sprintf(temp, "---你确认要将<%s>群的群主给<%s>吗?(y/n)", group_name, buf);
                    Write(cm.cfd, temp);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "n") == 0)
                    {
                        continue;
                    }
                    else if(strcmp(buf, "y") == 0)
                    {
                        //将group_master设置为群主
                        //UserData中的password
                        sprintf(query_str, "update UserData set password = \"%s\" \
                        where username = \"%s\"", group_master, group_name);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                        //Group_name中的type
                        sprintf(query_str, "update %s set type = \"2\" \
                        where username = \"%s\"", group_name, group_master);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                        sprintf(temp, "---<%s>群群主已成功转交给<%s>\n", group_name, group_master);
                        Write(cm.cfd, temp);

                        break;
                    }
                    else
                    {
                        strcpy(temp, "---非法输入，请输入(y/n)");
                        Write(cm.cfd, temp);
                        continue;
                    }
                }
            }
            
            //把自己从Group_name中抹去
            sprintf(query_str, "delete from %s where username = \"%s\"", \
                                        group_name,         cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);            

            sprintf(temp, "---成功退出群<%s>---\n", group_name);
            Write(cm.cfd, temp);
            
            break;
        }
        else
        {
            strcpy(temp, "---非法输入，请输入(y/n)");
            Write(cm.cfd, temp);
            continue;
        }
    }

    pthread_exit(0);
}

void *Group_kick_member(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    int group_flag;
    int identity;
    int to_identity;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];
    char member_name[BUFSIZE];

    identity = cm.tocfd;
    strcpy(group_name, cm.tousername);


    while(1)
    {
        //先判断权限是否足够，只有管理员或群主才有踢人的权限
        if(identity == 0)
        {
            strcpy(temp, "---权限不足---\n");
            Write(cm.cfd, temp);
        }
        //打印出群成员
        Group_view_member((void *)&cm);

        strcpy(temp, "---请输入你要踢的群成员(q to quit):");
        Write(cm.cfd, temp);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            strcpy(temp, "---取消踢人---\n");
            Write(cm.cfd, temp);
            break;
        }
        flag = mysql_repeat(&cm.mysql, group_name, buf, 1);
        
        if(flag == 1)
        {
            strcpy(temp, "---群中没有此用户\n");
            Write(cm.cfd, temp);
            continue;
        }
        strcpy(member_name, buf);

        while(1)
        {
            sprintf(temp, "---你确定要踢出群成员<%s>吗?(y/n):", member_name);
            Write(cm.cfd, temp);
            Read(cm.cfd, buf, sizeof(buf), __LINE__);
            if(strcmp(buf, "y") == 0)
            {
                //再判断级别是否足够
                sprintf(query_str, "select status from UserData where username = \"%s\"", member_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    to_identity = atoi(row[0]);
                }
                if(identity <= to_identity)
                {
                    sprintf(temp, "---权限不足，踢出<%s>失败---\n", member_name);
                    Write(cm.cfd, temp);
                    break;
                }   

                //把member_name从Group_name中移除
                sprintf(query_str, "delete from %s where username = \"%s\"", \
                                            group_name,           member_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

            }
            else if(strcmp(buf, "n") == 0)
            {
                strcpy(temp, "---取消踢人---\n");
                Write(cm.cfd, temp);
                break;
            }
            else
            {
                strcpy(temp, "---非法输入，请输入(y/n)");
                Write(cm.cfd, temp);
                continue;
            }
        }
    }

    pthread_exit(0);
}

void *Group_Set_revoke_admini(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    int group_flag;
    int identity;
    int to_identity;
    int admini_num;

    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char query_str[BUFSIZE];
    char group_name[BUFSIZE];
    char member_name[BUFSIZE];
    char now_time[BUFSIZE];

    identity = cm.tocfd;
    strcpy(group_name, cm.tousername);


    while(1)
    {
        if(identity != 2)
        {
            strcpy(temp, "---你没有设置或撤销群管理员的权限---\n");
            Write(cm.cfd, temp);
            break;
        }

        //打印出当前的群管理员人数(*/10)
        admini_num = Group_view_admini_num((void *)&cm);
        sprintf(temp, "---管理员(%d/10)---\n", admini_num);
        Write(cm.cfd, temp);

        strcpy(temp, "---你想要(设置)/(撤销)管理员-(s/r/q):");
        Write(cm.cfd, temp);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            strcpy(temp, "---管理群管理员取消---\n");
            Write(cm.cfd, temp);

            break;
        }
        else if(strcmp(buf, "s") == 0)  //设置管理员
        {
            while(1)
            {
                admini_num = Group_view_admini_num((void *)&cm);
                if(admini_num >= 10)
                {
                    strcpy(temp, "---群管理员人数已达上限---\n");
                    Write(cm.cfd, temp);

                    break;
                }   

                //打印出群成员
                Group_view_member((void *)&cm);

                strcpy(temp, "---输入群成员名称以设置管理员(q to quit):");
                Write(cm.cfd, temp);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    strcpy(temp, "---设置群成员取消---\n");
                    Write(cm.cfd, temp);

                    break;
                }

                flag = mysql_repeat(&cm.mysql, group_name, buf, 1);
                if(flag == 1)
                {
                    strcpy(temp, "---用户名输入错误，群内不存在此用户\n");
                    Write(cm.cfd, temp);
                    continue;
                }

                strcpy(member_name, buf);
                //获得这个人的身份
                sprintf(query_str, "select type from %s where username = \"%s\"", \
                                                    group_name,          member_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    to_identity = atoi(row[0]);
                }
                if(to_identity == 1)
                {
                    sprintf(temp, "---<%s>已经是管理员---\n", member_name);
                    Write(cm.cfd, temp);

                    continue;
                }
                else if(to_identity == 2)
                {
                    strcpy(temp, "---你不可以设置自己为管理员---\n");
                    Write(cm.cfd, temp);

                    continue;
                }
                else if(to_identity == 0)
                {
                    //把member_name设置为type = 1
                    sprintf(query_str, "update %s set type = \"1\" where username = \"%s\"", \
                                            group_name,                         member_name);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                    sprintf(temp, "---<%s>成为新的管理员\n", member_name);
                    Write(cm.cfd, temp);

                    //把设置管理员的消息发送给OffLineMes
                    sprintf(temp, "你成为群<%s>的管理员", group_name);
                    sprintf(query_str, "insert into OffLineMes values \
                    (\"%s\", \"%s\", \"%s\", \"%s\", \"2\")", \
                    get_time(now_time), group_name, member_name, temp);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                    continue;
                }
            }
        }
        else if(strcmp(buf, "r") == 0)  //撤销管理员
        {
            while(1)
            {
                admini_num = Group_view_admini_num((void *)&cm);
                if(admini_num <= 0)
                {
                    strcpy(temp, "---本群暂无管理员---\n");
                    Write(cm.cfd, temp);

                    break;
                }

                //打印出群成员
                Group_view_member((void *)&cm);

                strcpy(temp, "---请输入群管理员昵称以撤销(q to quit):");
                Write(cm.cfd, temp);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    strcpy(temp, "---取消撤销管理员---\n");
                    Write(cm.cfd, temp);
                    break;
                }

                flag = mysql_repeat(&cm.mysql, group_name, buf, 1);
                if(flag == 1)
                {
                    strcpy(temp, "---输入错误，群内没有此成员---\n");
                    Write(cm.cfd, temp);

                    continue;
                }
                strcpy(member_name, buf);
                //判断member_name是否是管理员
                sprintf(query_str, "select type from %s where username = \"%s\"", \
                                                    group_name,         member_name);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    to_identity = atoi(row[0]);
                }
                if(to_identity == 2)
                {
                    strcpy(temp, "---你不能撤销自己的群主---\n");
                    Write(cm.cfd, temp);

                    continue;
                }
                else if(to_identity == 1)
                {
                    //将Group_name里member_name的type改称0
                    sprintf(query_str, "update %s set type = \"0\" where username = \"%s\"", \
                                            group_name,                           member_name);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                    sprintf(temp, "---<%s>已被设置为管理员---\n");
                    Write(cm.cfd, temp);
                    
                    continue;
                }
                else if(to_identity == 0)
                {
                    strcpy(temp, "---<%s>不是管理员---\n");
                    Write(cm.cfd, temp);

                    continue;
                }
            }
        }
        else
        {
            strcpy(temp, "---非法输入，请输入(s/r/q)---\n");
            Write(cm.cfd, temp);

            continue;
        }
        
    }

    pthread_exit(0);
}

int Group_view_admini_num(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    pthread_t thid;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int admini_num; 

    char query_str[BUFSIZE];
    char group_name[BUFSIZE];

    strcpy(group_name, cm.tousername);

    sprintf(query_str, "select * from %s where type = \"1\"", group_name);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    admini_num = 0;
    while(row = mysql_fetch_row(res))
    {
        admini_num++;
    }

    return admini_num;
}