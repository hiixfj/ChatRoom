#include "func.h"

extern pthread_mutex_t mutex;

extern int num_birth;
extern int clients[1000];

void *serv_new_client(void *arg)
{
    pthread_detach(pthread_self());

    pthread_t thid;
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    int rows;
    MYSQL_ROW row;
    MYSQL_RES *res;

    int flag;
    char username[20];
    char password[20];
    char nickname[20];
    char mibao[20];

    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char duff[BUFSIZE];

    while(1)
    {   
        strcpy(duff, "[1] 登陆\n[2] 注册\n[3] 找回密码\n[q] 关闭应用\n");
        Write(cm.cfd, duff);
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "1") == 0)         //登陆
        {
            strcpy(duff, "---请输入账号(q to quit):");
            Write(cm.cfd, duff);
            while(1)        //循环输入得到一个已被注册的用户名
            {
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                if(flag == 1)
                {
                    strcpy(duff, "---不存在此用户名，请重新输入(q to quit):");
                    Write(cm.cfd, duff);
                    continue;
                }
                strcpy(username, buf);
                
                strcpy(duff, "---请输入密码(q to quit):");
                Write(cm.cfd, duff);
                while(1)        //循环输入得到密码，如果输入正确的话进入用户界面
                {
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    // flag = mysql_repeat(&cm.mysql, "UserData", buf, 2);
                    sprintf(query_str, "select * from UserData where username = \"%s\"", username);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res = mysql_store_result(&cm.mysql);
                    if(res == NULL)
                    {
                        my_err("mysql_store_result error", __LINE__);
                    }
                    flag = 1;
                    while(row = mysql_fetch_row(res))
                    {
                        if(strcmp(buf, row[1]) == 0)
                        {
                            flag = 0;
                        }
                    }
                    if(flag == 1)
                    {
                        strcpy(duff, "---密码输入错误，请重新输入(q to quit):");
                        Write(cm.cfd, duff);
                        continue;
                    }
                    //执行到这里说明账号密码输入正确
                    //把UserData中的status改为1
                    //然后来进入用户界面
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update UserData set status = 1 where username = \"%s\"", username);
                    rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                    if(rows != 0)
                    {
                        printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                        my_err("mysql_real_query error", __LINE__);
                    }
                    //登陆成功时，把当前的cfd更新至UserData中
                    sprintf(query_str, "update UserData set cfd = %d where username = \"%s\"", cm.cfd, username);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                    strcpy(cm.username, username);
                    if(pthread_create(&thid, NULL, func_yonghu, (void *)&cm) == -1)
                    {
                        my_err("pthread_create error", __LINE__);
                    }
                    pthread_join(thid, NULL);

                    break;
                }
                break;
            }
        }
        else if(strcmp(buf, "2") == 0)    //注册
        {
            strcpy(duff, "---请输入账号(q to quit):");
            Write(cm.cfd, duff);
            while(1)        //循环输入一个未被注册的用户名
            {
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                if(flag == 0)
                {
                    strcpy(duff, "---该用户名已被注册，请重新输入(q to quit):");
                    Write(cm.cfd, duff);
                    continue;
                }
                strcpy(username, buf);
                // printf("username = %s\n", username);
                strcpy(duff, "---请输入密码(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                strcpy(password, buf);
                // printf("password = %s\n", password);
                strcpy(duff, "---请输入昵称(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                strcpy(nickname, buf);
                // printf("nickname = %s\n", nickname);
                strcpy(duff, "---请输入密保(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                strcpy(mibao, buf);
                // printf("mibao = %s\n", mibao);

                //所有信息输入完毕，将得到的信息更新进UserData中
                memset(query_str, 0, sizeof(query_str));
                sprintf(query_str, "insert into UserData values\
                (\"%s\", \"%s\", \"%s\", \"%s\", \"%d\", \"0\", \"%d\", \"0\")", \
                username, password, nickname, mibao, num_birth, cm.cfd);
                rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                //更新num_birth的值
                pthread_mutex_lock(&mutex);
                    num_birth++;
                pthread_mutex_unlock(&mutex);
                
                //为这个新用户建一个好友列表（表）
                memset(query_str, 0, sizeof(query_str));
                sprintf(query_str, "create table %s(username varchar(20), num double)", username);
                rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                strcpy(duff, "---注册成功\n");
                Write(cm.cfd, duff);
                break;
            }
        }
        else if(strcmp(buf, "3") == 0)    //找回密码
        {
            while(1)        //循环得到UserData中存在的username
            {
                strcpy(duff, "请输入要找回的账号(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                if(flag == 1)
                {   
                    strcpy(duff, "---没有你要找回的账号");
                    Write(cm.cfd, duff);
                    continue;
                }
                strcpy(username, buf);
                

                while(1)        //循环得到UserData中存在的密保
                {
                    strcpy(duff, "---请输入密保(q to quit):");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    flag = mysql_repeat(&cm.mysql, "UserData", buf, 4);
                    if(flag == 1)
                    {
                        strcpy(duff, "---密保错误");
                        Write(cm.cfd, duff);
                        continue;
                    }
                }

                //账号密保均输入正确
                //接下来开始设置新的密码
                while(1)
                {
                    strcpy(duff, "---请输入新密码(q to quit):");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    strcpy(password, buf);
                    strcpy(duff, "---请再次输入密码(q to quit):");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    if(strcmp(buf, password) != 0)
                    {
                        strcpy(duff, "---两次密码输入不同");
                        Write(cm.cfd, duff);
                        continue;
                    }

                    //把新的密码更新到UserData中
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update UserData set password = \"%s\" where username = \"%s\"", \
                                                                        password, username);
                    rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                    if(rows != 0)
                    {
                        printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                        my_err("mysql_real_query error", __LINE__);
                    }
                    strcpy(duff, "---修改密码成功\n");
                    Write(cm.cfd, duff);
                    break;
                }
                break;
            }
        }
        else if(strcmp(buf, "q") == 0)    //关闭子线程
        {
            break;
        }
    }

    pthread_exit(0);
}

void *func_yonghu(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;
    
    int rows;
    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    pthread_t thid;

    int i, j;
    int choose;
    int flag;
    int shield_flag;

    char query_str[BUFSIZE];
    char buf[BUFSIZE];
    char temp[BUFSIZE];
    char now_time[BUFSIZE];
    char duff[BUFSIZE];

    int newsnum = 0;

    while(1)
    {
        //打印自己
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "------%s------\n", cm.username);
        Write(cm.cfd, temp);
        memset(temp, 0, sizeof(temp));
        //打印自己的未读消息news
        //得先找到自己的未读消息数newsnum
        newsnum = mysql_inquire_newsnum(&cm.mysql, cm.username, __LINE__);
        sprintf(temp, "------news(%d)(v to view)------\n", newsnum);
        Write(cm.cfd, temp);
        memset(temp, 0, sizeof(temp));
        //打印用户界面选项
        strcpy(duff, "[a] 好友列表\n[b] 添加好友\n[c] 群列表\n[d] 群选项\n[s] 刷新列表\n[q] 退出\n");
        Write(cm.cfd, duff);
        // strcpy(duff, "[b] 添加好友\n");
        // Write(cm.cfd, duff);
        // strcpy(duff, "[c] 群列表\n");
        // Write(cm.cfd, duff);
        // strcpy(duff, "[d] 群选项\n");
        // Write(cm.cfd, duff);
        // strcpy(duff, "[s] 刷新列表\n");
        // Write(cm.cfd, duff);
        // strcpy(duff, "[q] 退出\n");
        // Write(cm.cfd, duff);

        Read(cm.cfd, buf, sizeof(buf), __LINE__);

        if(strcmp(buf, "v") == 0)
        {
            while(1)
            {
                i = 1;
                j = 1;
                strcpy(duff, "---输入序号以处理消息(q to quit)\n");
                Write(cm.cfd, duff);
                //
                //打印出自己的未读消息列表
                //
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "select * from OffLineMes where touser = \"%s\"", cm.username);
                rows = mysql_real_query(&cm.mysql, temp, strlen(temp));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    if(atoi(row[4]) == 6)
                    {
                        memset(temp, 0, sizeof(temp));
                        sprintf(temp, "[%d]-<%s>-(%s)发送来了文件:%s", i++, row[0], row[1], row[3]);
                        Write(cm.cfd, temp);

                        continue;
                    }
                    memset(temp, 0, sizeof(temp));
                    sprintf(temp, "[%d]-<%s>-(%s)---%s\n", i++, row[0], row[1], row[3]);
                    Write(cm.cfd, temp);
                }
                //找到自己管理的群，然后在OffLineMes中找touser = 这个群的消息，然后给自己打印出来
                sprintf(query_str, "select * from %s where num = \"3\" or num = \"4\"", cm.username);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    sprintf(query_str, "select * from OffLineMes \
                                    where touser = \"%s\"", row[0]);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        my_err("mysql_store_result error", __LINE__);
                    }
                    while(row2 = mysql_fetch_row(res2))
                    {
                        sprintf(temp, "[%d]-<%s>-(%s)---%s", i++, row2[0], row2[1], row2[3]);
                        Write(cm.cfd, temp);
                    }
                }
                

                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    //打印出用户界面
                    break;
                }
                choose = atoi(buf);
                if(choose == 0)
                {
                    continue;
                }
                else if(choose >= i)
                {
                    strcpy(duff, "---输入超出范围，请重新输入:");
                    Write(cm.cfd, duff);
                    continue;
                }
                //再次遍历一遍自己的未读消息
                //找到自己选择的消息并处理
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "select * from OffLineMes where touser = \"%s\"", cm.username);
                rows = mysql_real_query(&cm.mysql, temp, strlen(temp));
                if(rows != 0)
                {
                    printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                    my_err("mysql_real_query error", __LINE__);
                }
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    if(j++ == choose)
                    {
                        if(atoi(row[4]) == 1)       //加好友型消息
                        {
                            while(1)
                            {
                                strcpy(duff, "------同意(t)/拒绝(f)(q to quit)------\n");
                                Write(cm.cfd, duff);
                                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                                if(strcmp(buf, "t") == 0)
                                {
                                    //给自己一个回馈
                                    strcpy(duff, "---已接受\n");
                                    Write(cm.cfd, duff);

                                    //给加好友的人一个回馈
                                    sprintf(query_str, "insert into OffLineMes values\
                                    (\"%s\", \"%s\", \"%s\", \"%s接受了你的好友请求\", 2)", \
                                    get_time(now_time), row[2], row[1], row[2]);
                                    rows = mysql_real_query(&cm.mysql, query_str, strlen(query_str));
                                    if(rows != 0)
                                    {
                                        printf("%d:error:%s", __LINE__, mysql_error(&cm.mysql));
                                        my_err("mysql_real_query error", __LINE__);
                                    }
                                    memset(query_str, 0, sizeof(query_str));

                                    //给加好友的人newsnum+1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[1], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                                    ++newsnum,              row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把自己的newsnum-1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[2], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                                    --newsnum,              row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把这条消息从自己的未读消息队列中抹去
                                    sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把好友加入自己的好友列表
                                    sprintf(query_str, "insert into %s values(\"%s\", \"1\")", row[2], row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把自己加入对方好友列表
                                    sprintf(query_str, "insert into %s values(\"%s\", \"1\")", row[1], row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));


                                    break;
                                }
                                else if(strcmp(buf, "f") == 0)
                                {
                                    //给自己一个反馈
                                    strcpy(duff, "---已拒绝\n");
                                    Write(cm.cfd, duff);

                                    //给对方一个反馈
                                    sprintf(query_str, "insert into OffLineMes values\
                                    (\"%s\", \"%s\", \"%s\", \"%s拒绝了你的好友请求\", 2)", \
                                    get_time(now_time), row[2], row[1], row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //把这条消息从OffLineMes中抹去
                                    sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //获得自己的newsnum-1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[2], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = \"%d\" where username = \"%s\"", \
                                                                                        --newsnum,              row[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    //获得对方的newsnum+1
                                    newsnum = mysql_inquire_newsnum(&cm.mysql, row[1], __LINE__);
                                    sprintf(query_str, "update UserData set newsnum = \"%d\" where username = \"%s\"", \
                                                                                        ++newsnum,              row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                                    memset(query_str, 0, sizeof(query_str));

                                    break;
                                }
                                else if(strcmp(buf, "q") == 0)
                                {
                                    break;
                                }
                                else
                                {
                                    strcpy(duff, "---请输入t/f/q---\n");
                                    Write(cm.cfd, duff);
                                    continue;   
                                }
                            }
                            break;
                        }
                        else if(atoi(row[4]) == 2)  //只读型消息
                        {
                            //获取自己的newsnum-1
                            newsnum = mysql_inquire_newsnum(&cm.mysql, row[2], __LINE__);
                            sprintf(query_str, "update UserData set newsnum = \"%d\" where username = \"%s\"", \
                                                                                --newsnum,              row[2]);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            memset(query_str, 0, strlen(query_str));

                            //从OffLineMes中移除这条消息
                            sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            memset(query_str, 0, strlen(query_str));
                            
                            break;
                        }
                        else if(atoi(row[4]) == 0)  //聊天型消息,进入与对方的聊天框
                        {
                            strcpy(cm.tousername, row[1]);
                            
                            //判断对方是否屏蔽自己
                            shield_flag = 1;
                            sprintf(query_str, "select num from %s where username = \"%s\"", \
                                                                cm.tousername,    cm.username);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            res2 = mysql_store_result(&cm.mysql);
                            if(res2 == NULL)
                            {
                                my_err("mysql_store_result error", __LINE__);
                            }
                            while(row2 = mysql_fetch_row(res2))
                            {
                                if(atoi(row2[0]) == 0)
                                {
                                    strcpy(temp, "---对方已将你屏蔽\n");
                                    Write(cm.cfd, temp);
                                    shield_flag = 0;
                                }
                            }
                            if(shield_flag == 0)
                            {
                                break;
                            }

                            if(pthread_create(&thid, NULL, func_private_chat, (void *)&cm) == -1)
                            {
                                my_err("pthread_create error", __LINE__);
                            }
                            pthread_join(thid, NULL);

                            break;

                            // //判断对方是否在线
                            // sprintf(query_str, "select status from UserData where username = \"%s\"", cm.tousername);
                            // MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            // res2 = mysql_store_result(&cm.mysql);
                            // if(res2 == NULL)
                            // {
                            //     my_err("mysql_store_result error", __LINE__);
                            // }
                            // while(row2 = mysql_fetch_row(res2))
                            // {
                            //     if(atoi(row2[0]) == 0)
                            //     {
                            //         //进入留言线程
                            //         if(pthread_create(&thid, NULL, func_liuyan, (void *)&cm) == -1)
                            //         {
                            //             my_err("pthread_create error", __LINE__);
                            //         }
                            //         pthread_join(thid, NULL);
                            //     }
                            //     else if(atoi(row2[0]) == 1)
                            //     {
                            //         //进入聊天线程
                            //         if(pthread_create(&thid, NULL, func_liaotian, (void *)&cm) == -1)
                            //         {
                            //             my_err("pthread_create error", __LINE__);
                            //         }
                            //         pthread_join(thid, NULL);
                            //     }
                            // }

                            // //更新自己的newsnum-1
                            // newsnum = mysql_inquire_newsnum(&cm.mysql, cm.username, __LINE__);
                            // sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                            //                                                 --newsnum,          cm.username);
                            // MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                            // //将OffLineMes中---
                            // //---关于inuser = cm.tousername and touser = cm.username and type = 0的消息删除
                            // sprintf(query_str, "delete from OffLineMes \
                            // where inuser = \"%s\" and touser = \"%s\" and type = 0", \
                            //                 cm.tousername,      cm.username);
                            // MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);


                            // break;
                        }
                        else if(atoi(row[4]) == 3)  //群只读类消息
                        {
                            //从OffLineMes中移除这条消息
                            sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            break;
                        }
                        else if(atoi(row[4]) == 4)  //群聊天消息
                        {
                            //通过消息的时间找到这个群聊的名称
                            sprintf(query_str, "select * from HisData \
                            where time = \"%s\"", row[0]);
                            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                            res2 = mysql_store_result(&cm.mysql);
                            if(res2 == NULL)
                            {
                                my_err("mysql_store_result error", __LINE__);
                            }
                            while(row2 = mysql_fetch_row(res2))
                            {
                                strcpy(cm.tousername, row2[2]);
                            }
                            
                            //进入聊天界面
                            if(pthread_create(&thid, NULL, Group_chat, (void *)&cm) == -1)
                            {
                                my_err("pthread_create error", __LINE__);
                            }
                            pthread_join(thid, NULL);

                            break;
                        }
                        else if(atoi(row[4]) == 5)  //加群请求
                        {
                            //基本上进不来
                            //但还是写一写
                            while(1)
                            {
                                strcpy(temp, "------同意(t)/拒绝(f)(q to quit)------\n");
                                Write(cm.cfd, temp);
                                Read(cm.cfd, buf, sizeof(buf), __LINE__);

                                if(strcmp(buf, "t") == 0)
                                {
                                    //将对方加入"Group"表中，type = 0
                                    sprintf(query_str, "insert into %s values \
                                    (\"%s\", \"0\")", row[2], row[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //把新成员加入群聊的消息广播给所有群成员
                                    sprintf(temp, "<%s>-<%s>加入了群聊(%s)", row[0], row[1], row[2]);
                                    strcpy(cm.tousername, row[2]);
                                    Group_broadcast((void *)&cm, temp, 3);

                                    //将这条消息从OffLineMes中抹去
                                    sprintf(query_str, "delete from OffLineMes \
                                                where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    break;
                                }
                                else if(strcmp(buf, "f") == 0)
                                {
                                    //给发送请求的用户一个通知
                                    sprintf(temp, "---<%s>:你的加群审核未通过", row[2]);
                                    sprintf(query_str, "insert into OffLineMes values \
                                    (\"%s\", \"%s\", \"%s\", \"%s\", \"3\")", \
                                    get_time(now_time), row[2], row[1], temp);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //将这条消息从OffLineMes中移除
                                    sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    break;
                                }
                                else if(strcmp(buf, "q") == 0)
                                {
                                    break;
                                }
                                else
                                {
                                    strcpy(temp, "---非法输入，请输入(t/f/q)---\n");
                                    Write(cm.cfd, temp);
                                    continue;
                                }
                            }
                        }
                        else if(atoi(row[4]) == 6)  //发文件请求
                        {
                            while(1)
                            {
                                strcpy(temp, "---同意(t)/拒绝(f)(q to quit)---");
                                Write(cm.cfd, temp);
                                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                                if(strcmp(buf, "q") == 0)
                                {
                                    break;       
                                }
                                else if(strcmp(buf, "t") == 0)
                                {
                                    //把文件名赋值给cm.tousername
                                    strcpy(cm.tousername, row[3]);
                                    if(pthread_create(&thid, NULL, func_Friend_recv_file, (void *)&cm) == -1)
                                    {
                                        my_err("pthread_create error", __LINE__);
                                    }
                                    pthread_join(thid, NULL);

                                    //给自己一个反馈
                                    sprintf(temp, "---成功接收文件<%s>\n", row[3]);
                                    Write(cm.cfd, temp);

                                    //给对方一个反馈
                                    sprintf(temp, "<%s>接收了文件<%s>", cm.username, row[3]);
                                    sprintf(query_str, "insert into OffLineMes values \
                                    (\"%s\", \"%s\", \"%s\", \"%s\", \"2\")", \
                                    get_time(now_time), cm.username, row[1], temp);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //把这条信息从OffLineMes中抹去
                                    sprintf(query_str, "delete from OffLineMes \
                                    where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    break;
                                }
                                else if(strcmp(buf, "f") == 0)
                                {
                                    //给自己一个反馈
                                    sprintf(temp, "---拒绝接收文件%s\n", row[3]);
                                    Write(cm.cfd, temp);

                                    //给对方一个反馈
                                    sprintf(temp, "<%s>拒绝接收文件<%s>", cm.username, row[3]);
                                    sprintf(query_str, "insert into OffLineMes values \
                                    (\"%s\", \"%s\", \"%s\", \"%s\", \"2\")", \
                                    get_time(now_time), cm.username, row[1], temp);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //在系统的文件缓存区中删除这个文件
                                    sprintf(temp, "./file_buf/%s", row[3]);
                                    if(unlink(temp) < 0)
                                    {
                                        my_err("unlink error", __LINE__);
                                    }
                                    printf("file:%s:unlinked\n", row[3]);

                                    //从OffLineMes中抹去这条消息
                                    sprintf(query_str, "delete from OffLineMes \
                                    where time = \"%s\"", row[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    break;
                                }
                                else
                                {
                                    strcpy(temp, "---非法输入---\n");
                                    Write(cm.cfd, temp);
                                    
                                    continue;
                                }
                            }
                        }
                    }
                }
                //上面的循环无法遍历到加群请求的消息
                //由下面的循环再作遍历
                sprintf(query_str, "select * from %s where num = \"3\" or num = \"4\"", cm.username);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    sprintf(query_str, "select * from OffLineMes \
                                    where touser = \"%s\"", row[0]);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        my_err("mysql_store_result error", __LINE__);
                    }
                    while(row2 = mysql_fetch_row(res2))
                    {
                        //真正是从这里写入的
                        if(j++ == choose)
                        {
                            while(1)
                            {
                                strcpy(temp, "------同意(t)/拒绝(f)(q to quit)------\n");
                                Write(cm.cfd, temp);
                                Read(cm.cfd, buf, sizeof(buf), __LINE__);

                                if(strcmp(buf, "t") == 0)
                                {
                                    //给自己一个反馈
                                    sprintf(temp, "---已接受<%s>的加群请求---\n", row2[1]);
                                    Write(cm.cfd, temp);

                                    //将对方加入"Group"表中，type = 0
                                    sprintf(query_str, "insert into %s values \
                                    (\"%s\", \"0\")", row2[2], row2[1]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //把群聊名加入对方的表中
                                    sprintf(query_str, "insert into %s values \
                                    (\"%s\", \"2\")", row2[1], row2[2]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //把新成员加入群聊的消息广播给所有群成员
                                    sprintf(temp, "<%s>-<%s>加入了群聊(%s)", get_time(now_time), row2[1], row2[2]);
                                    strcpy(cm.tousername, row2[2]);
                                    Group_broadcast((void *)&cm, temp, 3);

                                    //将这条消息从OffLineMes中抹去
                                    sprintf(query_str, "delete from OffLineMes \
                                                where time = \"%s\"", row2[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    break;
                                }
                                else if(strcmp(buf, "f") == 0)
                                {
                                    //给自己一个反馈
                                    sprintf(temp, "---已拒绝<%s>的加群请求---\n", row2[1]);
                                    Write(cm.cfd, temp);

                                    //给发送请求的用户一个通知
                                    sprintf(temp, "---<%s>:你的加群审核未通过", row2[2]);
                                    sprintf(query_str, "insert into OffLineMes values \
                                    (\"%s\", \"%s\", \"%s\", \"%s\", \"3\")", \
                                    get_time(now_time), row2[2], row2[1], temp);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    //将这条消息从OffLineMes中移除
                                    sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row2[0]);
                                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                                    break;
                                }
                                else if(strcmp(buf, "q") == 0)
                                {
                                    break;
                                }
                                else
                                {
                                    strcpy(temp, "---非法输入，请输入(t/f/q)---\n");
                                    Write(cm.cfd, temp);
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if(strcmp(buf, "a") == 0)
        {
            while(1)
            {
                //列出自己的好友
                memset(query_str, 0, sizeof(query_str));
                sprintf(query_str, "select * from %s where num = \"0\" or num = \"1\"", cm.username);
                MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                res = mysql_store_result(&cm.mysql);
                if(res == NULL)
                {
                    my_err("mysql_store_result error", __LINE__);
                }
                while(row = mysql_fetch_row(res))
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "select status from UserData where username = \"%s\"", row[0]);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        my_err("mysql_real_query error", __LINE__);
                    } 
                    while(row2 = mysql_fetch_row(res2))
                    {
                        if(atoi(row2[0]) == 1)
                        {
                            memset(temp, 0, sizeof(temp));
                            sprintf(temp, "------%s---在线\n", row[0]);
                            Write(cm.cfd, temp);
                        }
                        else if(atoi(row2[0]) == 0)
                        {
                            memset(temp, 0, sizeof(temp));
                            sprintf(temp, "------%s---离线\n", row[0]);
                            Write(cm.cfd, temp);
                        }
                    }
                }

                //输入好友的用户名进入聊天界面
                strcpy(duff, "---输入好友的用户名进入聊天界面(q to quit):");
                Write(cm.cfd, duff);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "q") == 0)
                {
                    break;
                }
                flag = mysql_repeat(&cm.mysql, cm.username, buf, 1);
                if(flag == 1)
                {
                    strcpy(duff, "---你没有此好友，请重新输入\n");
                    Write(cm.cfd, duff);
                    continue;
                }
                else if(flag == 0)
                {
                    strcpy(cm.tousername, buf);

                    //判断对方是否屏蔽自己
                    shield_flag = 1;
                    sprintf(query_str, "select num from %s where username = \"%s\"", \
                                                        cm.tousername,    cm.username);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    res2 = mysql_store_result(&cm.mysql);
                    if(res2 == NULL)
                    {
                        my_err("mysql_store_result error", __LINE__);
                    }
                    while(row2 = mysql_fetch_row(res2))
                    {
                        if(atoi(row2[0]) == 0)
                        {
                            strcpy(temp, "---对方已将你屏蔽\n");
                            Write(cm.cfd, temp);
                            shield_flag = 0;
                        }
                    }
                    if(shield_flag == 0)
                    {
                        continue;
                    }

                    //
                    //由此进入聊天界面
                    //每次发送的消息先由聊天线程处理判断是否是预设好的参数
                    //如果不是的话，再把消息传给一个消息发送函数，专门用于发送数据
                    if(pthread_create(&thid, NULL, func_private_chat, (void *)&cm) == -1)
                    {
                        my_err("pthread_create error", __LINE__);
                    }
                    pthread_join(thid, NULL);
                    continue;


                    // //判断对方是否在线
                    // sprintf(query_str, "select status from UserData where username = \"%s\"", cm.tousername);
                    // printf("%s\n", query_str);
                    // MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    // res2 = mysql_store_result(&cm.mysql);
                    // if(res2 == NULL)
                    // {
                    //     printf("%d:error:%s\n", __LINE__, mysql_error(&cm.mysql));
                    //     my_err("mysql_store_result error", __LINE__);
                    // }
                    // while(row2 = mysql_fetch_row(res2))
                    // {
                    //     if(atoi(row2[0]) == 0)
                    //     {
                    //         //进入留言线程
                    //         if(pthread_create(&thid, NULL, func_liuyan, (void *)&cm) == -1)
                    //         {
                    //             my_err("pthread_create error", __LINE__);
                    //         }
                    //         pthread_join(thid, NULL);
                    //     }
                    //     else if(atoi(row2[0]) == 1)
                    //     {
                    //         //进入聊天线程
                    //         if(pthread_create(&thid, NULL, func_liaotian, (void *)&cm) == -1)
                    //         {
                    //             my_err("pthread_create error", __LINE__);
                    //         }
                    //         pthread_join(thid, NULL);
                    //     }
                    // }
                    // break;
                }
            }
        }
        else if(strcmp(buf, "b") == 0)
        {
            strcpy(duff, "---添加好友(1)/(q to quit)---\n");
            Write(cm.cfd, duff);
            Read(cm.cfd, buf, sizeof(buf), __LINE__);
            if(strcmp(buf, "q") == 0)
            {
                continue;
            }
            else if(strcmp(buf, "1") == 0)
            {
                while(1)
                {
                    strcpy(duff, "---请输入用户名:");
                    Write(cm.cfd, duff);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);
                    if(strcmp(buf, "q") == 0)
                    {
                        break;
                    }
                    flag = mysql_repeat(&cm.mysql, "UserData", buf, 1);
                    if(flag == 0)
                    {
                        strcpy(duff, "---已发送好友请求\n");
                        Write(cm.cfd, duff);

                        //把这条消息发送到OffLineMes中
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(query_str, "insert into OffLineMes values\
                        (\"%s\", \"%s\", \"%s\", \"%s向你发来好友请求\", \"1\")", \
                        get_time(now_time), cm.username, buf, cm.username);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                        //把对方newsnum+1
                        newsnum = mysql_inquire_newsnum(&cm.mysql, buf, __LINE__);
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(query_str, "update UserData set newsnum = %d where username = \"%s\"", \
                                                                            ++newsnum,          buf);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    }
                    else if(flag == 1)
                    {
                        strcpy(duff, "---不存在此用户\n");
                        Write(cm.cfd, duff);
                        continue;
                    }
                }
            }
        }
        else if(strcmp(buf, "c") == 0)
        {
            if(pthread_create(&thid, NULL, func_group_list, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "d") == 0)
        {
            if(pthread_create(&thid, NULL, func_Group_options, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
        }
        else if(strcmp(buf, "q") == 0)//
        {
            //把用户的status设置为0
            sprintf(query_str, "update UserData set status = 0 where username = \"%s\"", cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            memset(query_str, 0, sizeof(query_str));
            break;
        }
        else if(strcmp(buf, "s") == 0)//
        {
            continue;
        }
    }

    pthread_exit(0);
}

void *func_liaotian(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    pthread_t thid;

    int newsnum;

    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char temp[BUFSIZE];
    char now_time[BUFSIZE];

    memset(buf, 0, sizeof(buf));
    memset(query_str, 0, sizeof(query_str));
    memset(temp, 0, sizeof(temp));


    sprintf(temp, "------%s(\"quit-exit\" to quit)------\n", cm.tousername);
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //给出好友选项参数
    //-hisdata  查看消息记录
    strcpy(temp, "------(-hisdata to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //-Friends_permissions  管理好友权限
    strcpy(temp, "------(-Friends_permissions to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));


    //先把对方发送过来的未读消息全部打印出来
    sprintf(query_str, "select * from OffLineMes where touser = \"%s\"", cm.username);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
        my_err("mysql_store_result error", __LINE__);
    while(row = mysql_fetch_row(res))
    {
        sprintf(temp, "<%s>-<%s>:%s\n", row[0], row[1], row[3]);
        Write(cm.cfd, temp);
        memset(temp, 0, sizeof(temp));
    }
    memset(query_str, 0, sizeof(query_str));

    //获得对方的套接字
    sprintf(query_str, "select cfd from UserData where username = \"%s\"", cm.tousername);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        cm.tocfd = atoi(row[0]);
    }

    //然后进入聊天
    //对方收到消息之后需要手动进入与自己的对话框才可以回复消息
    // printf("cm.cfd = %d\n", cm.cfd);
    // printf("cm.tocfd = %d\n", cm.tocfd);
    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "quit-exit") == 0)
        {
            break;
        }
        else if(strcmp(buf, "-hisdata") == 0)
        {
            //显示出历史消息记录
            sprintf(query_str, "select * from HisData \
            where inuser = \"%s\" and touser = \"%s\" or inuser = \"%s\" and touser = \"%s\"", \
                        cm.username, cm.tousername,                 cm.tousername, cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }
            while(row = mysql_fetch_row(res))
            {
                sprintf(temp, "<%s>-<%s>-><%s>:%s\n", row[0], row[1], row[2], row[3]);
                Write(cm.cfd, temp);
            }
            continue;
        }
        else if(strcmp(buf, "-Friends_permissions") == 0)   //好友权限管理
        {
            if(pthread_create(&thid, NULL, func_Friends_permissions, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }

        sprintf(temp, "<%s>-<%s>:%s", get_time(now_time), cm.username, buf);
        Write(cm.tocfd, temp);

        //将读到的内容加入聊天记录HisData中
        sprintf(query_str, "insert into HisData values\
        (\"%s\", \"%s\", \"%s\", \"%s\")", \
        now_time, cm.username, cm.tousername, buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }

    pthread_exit(0);
}

void *func_liuyan(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    pthread_t thid;

    int newsnum;

    char query_str[BUFSIZE];
    char buf[BUFSIZE];
    char now_time[BUFSIZE];
    char temp[BUFSIZE];

    memset(query_str, 0, sizeof(query_str));
    sprintf(query_str, "------%s(\"quit-exit\" to quit)------离线\n", cm.tousername);
    Write(cm.cfd, query_str);
    //给出好友选项参数
    //-hisdata  查看消息记录
    strcpy(temp, "------(-hisdata to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //-Friends_permissions  管理好友权限
    strcpy(temp, "------(-Friends_permissions to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));

    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "quit-exit") == 0)
        {
            break;        
        }
        else if(strcmp(buf, "-hisdata") == 0)
        {
            //显示出历史消息记录
            sprintf(query_str, "select * from HisData \
            where inuser = \"%s\" and touser = \"%s\" or inuser = \"%s\" and touser = \"%s\"", \
                        cm.username, cm.tousername,                 cm.tousername, cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }
            while(row = mysql_fetch_row(res))
            {
                sprintf(temp, "<%s>-<%s>-><%s>:%s\n", row[0], row[1], row[2], row[3]);
                Write(cm.cfd, temp);
            }
            continue;
        }
        else if(strcmp(buf, "-Friends_permissions") == 0)   //好友权限管理
        {
            if(pthread_create(&thid, NULL, func_Friends_permissions, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }


        //把消息加入未读消息队列OffLineMes中
        sprintf(query_str, "insert into OffLineMes values\
        (\"%s\", \"%s\", \"%s\", \"%s\", \"0\")", \
        get_time(now_time), cm.username, cm.tousername, buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        memset(query_str, 0, sizeof(query_str));
        //给消息接收人的newsnum+1
        newsnum = mysql_inquire_newsnum(&cm.mysql, cm.tousername, __LINE__);
        sprintf(query_str, "update UserData set newsnum = \"%d\"", ++newsnum);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
        memset(query_str, 0, sizeof(query_str));

        //把消息加入聊天记录HisData中
        sprintf(query_str, "insert into HisData values\
        (\"%s\", \"%s\", \"%s\", \"%s\")", \
        now_time, cm.username, cm.tousername, buf);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }

    pthread_exit(0);
}

void *func_private_chat(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    pthread_t thid;

    int newsnum;

    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char temp[BUFSIZE];
    char now_time[BUFSIZE];
    char friend[BUFSIZE];

    memset(buf, 0, sizeof(buf));
    memset(query_str, 0, sizeof(query_str));
    memset(temp, 0, sizeof(temp));

    strcpy(friend, cm.tousername);

    Friendchat_h((void *)&cm);

    //先把对方发来的未读消息全部打印出来
    sprintf(query_str, "select * from OffLineMes \
    where inuser = \"%s\" and touser = \"%s\" and type = \"0\"", friend, cm.username);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        sprintf(temp, "---<%s>-<%s>:%s", row[0], row[1], row[3]);
        Write(cm.cfd, temp);

        //将这条消息从OffLineMes中抹去
        sprintf(query_str, "delete from OffLineMes where time = \"%s\"", row[0]);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }

    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);

        if(strcmp(buf, "quit-exit") == 0)
        {
            break;
        }
        else if(strcmp(buf, "-hisdata") == 0)
        {
            sprintf(query_str, "select * from HisData \
            where inuser = \"%s\" and touser = \"%s\" or inuser = \"%s\" and touser = \"%s\"", \
            cm.username, friend, friend, cm.username);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
            res = mysql_store_result(&cm.mysql);
            if(res == NULL)
            {
                my_err("mysql_store_result error", __LINE__);
            }

            while(row = mysql_fetch_row(res))
            {
                sprintf(temp, "<%s>-<%s>:%s", row[0], row[1], row[3]);
                Write(cm.cfd, temp);                
            }

            continue;
        }
        else if(strcmp(buf, "-Friends_permissions") == 0)
        {
            if(pthread_create(&thid, NULL, func_Friends_permissions, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else if(strcmp(buf, "-h") == 0)
        {
            Friendchat_h((void *)&cm);
            continue;
        }
        else if(strcmp(buf, "-send_file") == 0)
        {
            //进入发文件线程
            if(pthread_create(&thid, NULL, func_Friend_send_file, (void *)&cm) == -1)
            {
                my_err("pthread_create error", __LINE__);
            }
            pthread_join(thid, NULL);
            continue;
        }
        else 
        {
            Friend_send_mes((void *)&cm, buf);
            continue;
        }

    }

    pthread_exit(0);
}

void Friend_send_mes(void *arg, char *q)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    int status;

    char temp[BUFSIZE];
    char friend[BUFSIZE];
    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char now_time[BUFSIZE];

    strcpy(buf, q);
    strcpy(friend, cm.tousername);

    //把消息发送给friend之前，先判断是否在线，再选择发送方式
    sprintf(query_str, "select * from UserData \
            where username = \"%s\"", friend);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        status = atoi(row[5]);
        cm.tocfd = atoi(row[6]);
    }
    if(status == 1)         //在线
    {
        //直接向对方的套接字发送消息
        sprintf(temp, "<%s>-<%s>:%s", get_time(now_time), cm.username, buf);
        Write(cm.tocfd, temp);
    }
    else if(status == 0)    //离线
    {
        //向OffLineMes中存入消息
        sprintf(query_str, "insert into OffLineMes values\
        (\"%s\", \"%s\", \"%s\", \"%s\", \"0\")", \
        get_time(now_time), cm.username, friend, buf);
        printf("%s\n", query_str);
        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    }
    //把消息存入消息记录HisData中
    sprintf(query_str, "insert into HisData values \
    (\"%s\", \"%s\", \"%s\", \"%s\")", now_time, cm.username, friend, buf);
    printf("%s\n", query_str);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

}

void Friendchat_h(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    char temp[BUFSIZE];

    sprintf(temp, "------%s(\"quit-exit\" to quit)------\n", cm.tousername);
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //给出好友选项参数
    strcpy(temp, "------(\"-send_file\" to send file)------\n");
    Write(cm.cfd, temp);
    //-hisdata  查看消息记录
    strcpy(temp, "------(\"-hisdata\" to view chat history)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    //-Friends_permissions  管理好友权限
    strcpy(temp, "------(\"-Friends_permissions\" set permission)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
    
    strcpy(temp, "------(\"-h\" for help)------\n");
    Write(cm.cfd, temp);
    memset(temp, 0, sizeof(temp));
}

void *func_Friends_permissions(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    char temp[BUFSIZE];
    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char now_time[BUFSIZE];


    //先看看好友是否已被屏蔽
    memset(query_str, 0, sizeof(query_str));
    sprintf(query_str, "select num from %s where username = \"%s\"", \
                                cm.username,            cm.tousername);
    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(&cm.mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        if(atoi(row[0]) == 1)
        {
            while(1)
            {
                strcpy(temp, "[1] 屏蔽好友信息\n[2] 删除好友\n[q] 返回");
                Write(cm.cfd, temp);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "1") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update %s set num = \"0\" where username = \"%s\"", \
                                            cm.username,                       cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已屏蔽好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "2") == 0)
                {
                    sprintf(temp, "---你确定要删除好友<%s>吗?<y/n>", cm.tousername);
                    Write(cm.cfd, temp);
                    Read(cm.cfd, buf, sizeof(buf), __LINE__);

                    if(strcmp(buf, "y") == 0)
                    {
                        //把对方从自己的好友列表中删除
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(query_str, "delete from %s where username = \"%s\"", \
                                                        cm.username,    cm.tousername);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                        
                        sprintf(temp, "---已删除好友:%s\n", cm.tousername);
                        Write(cm.cfd, temp);

                        //把自己从对方的好友列表中删除
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(query_str, "delete from %s where username = \"%s\"", \
                                                        cm.tousername,      cm.username);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                        //给对方一个反馈
                        memset(query_str, 0, sizeof(query_str));
                        sprintf(temp, "<%s>删除了你的好友", cm.username);
                        sprintf(query_str, "insert into OffLineMes values \
                        (\"%s\", \"%s\", \"%s\", \"%s\", \"2\")", \
                        get_time(now_time), cm.username, cm.tousername, temp);
                        MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);

                        break;
                    }
                    else if(strcmp(buf, "n") == 0)
                    {
                        strcpy(temp, "---删除好友取消---\n");
                        Write(cm.cfd, temp);

                        continue;
                    }
                    else
                    {
                        strcpy(temp, "---非法输入---\n");
                        Write(cm.cfd, temp);

                        continue;
                    }


                    
                }
                else if(strcmp(buf, "q") == 0)
                {
                    strcpy(temp, "---好友权限界面退出---\n");
                    Write(cm.cfd, temp);

                    break;
                }
                else
                {
                    strcpy(temp, "---输入不合规范，请输入1/2/q\n");
                    Write(cm.cfd, temp);
                    continue;
                }
            }
        }
        else if(atoi(row[0]) == 0)
        {
            while(1)
            {
                strcpy(temp, "[1] 屏蔽好友信息---解除\n[2] 删除好友\n[q] 返回\n");
                Write(cm.cfd, temp);
                Read(cm.cfd, buf, sizeof(buf), __LINE__);
                if(strcmp(buf, "1") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "update %s set num = \"1\" where username = \"%s\"", \
                                            cm.username,                       cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已解除屏蔽好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "2") == 0)
                {
                    memset(query_str, 0, sizeof(query_str));
                    sprintf(query_str, "delete from %s where username = \"%s\"", \
                                                    cm.username,    cm.tousername);
                    MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);
                    
                    sprintf(temp, "---已删除好友:%s\n", cm.tousername);
                    Write(cm.cfd, temp);
                }
                else if(strcmp(buf, "q") == 0)
                {
                    break;
                }
            }
        }
    }

    pthread_exit(0);
}

void *func_Friend_send_file(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    pthread_t thid;

    char temp[BUFSIZE];
    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char now_time[BUFSIZE];
    char friend[BUFSIZE];
    char* filename;
    char duff[BUFSIZE];
    char file_path[BUFSIZE];

    struct stat buffer;

    strcpy(friend, cm.tousername);

    strcpy(temp, "------发送文件------\n");
    Write(cm.cfd, temp);
    
    strcpy(temp, "-send_file");
    Write(cm.cfd, temp);

    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            break;
        }
        else if(strcmp(buf, "-start_send") == 0)
        {
            char name[100];
            //获取用户端传来的文件长度
            int len;
            Read(cm.cfd, buf, sizeof(buf), __LINE__);
            len = atoi(buf);
            //获取客户端传来的文件名于buf中
            Read(cm.cfd, buf, sizeof(buf), __LINE__);
            sprintf(temp, "./file_buf/%s", buf);
            printf("temp = %s\n", temp);
            strcpy(name, buf);
            //创建文件
            FILE *fp = fopen(temp, "wb");
            if (fp == NULL) 
            {
                perror("Can't open file");
                exit(1);
            }
            
            //把数据写入文件
            printf("Start receive file: %s from %s\n", temp, inet_ntoa(cm.clit_addr.sin_addr));
            Read(cm.cfd, buf, len, __LINE__);
            fwrite(buf, sizeof(char), len, fp);
            puts("Receive Success");

            //关闭文件
            fclose(fp);

            //把该文件名添加到OffLineMes中
            sprintf(query_str, "insert into OffLineMes values \
            (\"%s\", \"%s\", \"%s\", \"%s\", \"6\")", \
            get_time(now_time), cm.username, friend, name);
            MY_real_query(&cm.mysql, query_str, strlen(query_str), __LINE__);   

            printf("break?\n");
            break;
        }
    }

    pthread_exit(0);
}

void *func_Friend_recv_file(void *arg)
{
    struct cfd_mysql cm;
    cm = *(struct cfd_mysql *)arg;

    MYSQL_ROW row;
    MYSQL_RES *res;

    pthread_t thid;

    char temp[BUFSIZE];
    char buf[BUFSIZE];
    char query_str[BUFSIZE];
    char now_time[BUFSIZE];
    char friend[BUFSIZE];
    char filename[BUFSIZE];
    char duff[BUFSIZE];
    char file_path[BUFSIZE];

    struct stat buffer;

    strcpy(filename, cm.tousername);
    sprintf(file_path, "./file_buf/%s", filename);

    strcpy(temp, "------接收文件------\n");
    Write(cm.cfd, temp);
    strcpy(temp, "------输入\"-recv_file\"以接收文件(q to quit)\n");
    Write(cm.cfd, temp);

    while(1)
    {
        Read(cm.cfd, buf, sizeof(buf), __LINE__);
        if(strcmp(buf, "q") == 0)
        {
            break;
        }
        else if(strcmp(buf, "-recv_file") == 0)
        {
            Write(cm.cfd, buf);

            //把文件长度发送给客户端
            if(stat(file_path, &buffer) == -1)
            {
                my_err("stat error", __LINE__);
            }
            sprintf(buf, "%d", buffer.st_size);
            Write(cm.cfd, buf);
            //把文件名发送给客户端
            Write(cm.cfd, filename);

            //向套接字发送文件
            //打开要发送的文件
            // FILE *fp = fopen(file_path, "rb");
            int fp = open(file_path, O_CREAT|O_RDONLY, S_IWUSR|S_IRUSR);

            //读取并发送文件
            sendfile(cm.cfd, fp, 0, buffer.st_size);
            puts("Send Success");

            //删除这个临时文件
            if(unlink(file_path) == -1)
            {
                my_err("unlink error", __LINE__);
            }

            //关闭文件和套接字
            close(fp);

            break;
        }
    }

    pthread_exit(0);
}