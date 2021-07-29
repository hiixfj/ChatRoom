#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include "time.h"

#define MAXEVE 1024

struct cfd_mysql
{
    int cfd;
    MYSQL mysql;
    struct sockaddr_in clit_addr;
    char username[20];
    char tousername[20];
    int tocfd;
    int retval;
};

//每来一个用户就新建一个线程来这里
void *serv_new_client(void *arg);











//
//打印类函数
//
//欢迎界面信息
void welcome();
//用户界面信息
void welcome_1();
//好友界面信息-->好友列表
void welcome_friends();
//获取系统当前时间
char *get_time(char *now_time);

//
//引导类函数
//
//欢迎界面函数
void welcome_interface(int cfd);
//用户界面函数
void client_interface();
//好友界面函数
void friends_interface();


//
//错误处理类函数
//
void Write(int fd, const char *buf);
//read之前清空缓冲区buf
int Read(int fd, char *buf, size_t count, int line);


//
//实用类函数
//
//键入
int get_userinfo(char *buf, int len);

//把键入值发送给服务器，服务器与数据库中的数据进行比对，直到输入正确为止
void judge_userinfo(int fd, const char *string);

//解析客户端发来的请求,并把请求转化为整数
int parse_request(char *request);


void my_err(const char *str, const int line);


//
//mysql类函数
//
//向表中add数据
//str为insert语句
void mysql_add(MYSQL *mysql, const char *str, const struct sockaddr_in clit_addr, const char *table);

//建表
//str为建表语句
void mysql_build(MYSQL *mysql, const char *str);

//查表（查询str是否重复）
//string是表名
//str是待查询的字符串
//field是第几列
//有重复返回0,无重复返回1
int mysql_repeat(MYSQL *mysql, const char *string, const char *str, int field);

//查询newsnum
//username是待查询的用户名
int mysql_inquire_newsnum(MYSQL *mysql, const char *username, int line);

//封装好的mysql_real_query执行程序，无需错误处理
int MY_real_query(MYSQL *mysql, const char *q, unsigned long length, int line);

//server多线程函数
int huitui(const char *buf);
//输入q的话返回0
//其它返回1
int huitui_val(const char *buf);

void *func_zhuce(void *arg);
void *func_denglu(void *arg);
void *func_zhaohui(void *arg);
void *func_yonghu(void *arg);
void *func_liaotian(void *arg);
void *func_liuyan(void *arg);
//好友权限管理
void *func_Friends_permissions(void *arg);
//群选项Group options---[1]创建群 [2]解散群 [3]申请加群
void *func_Group_options(void *arg);
void *Group_create(void *arg);
void *Group_disband(void *arg);
void *Group_apply(void *arg);

void *func_group_list(void *arg);
void *Group_chat(void *arg);
//打印帮助菜单
void Group_h(void *arg, int identity);
//查看群成员，调用之前需要给cm.tousername赋值为群聊名
void Group_view_member(void *arg);
//查看群管理员，调用之前需要给cm.tousername赋值为群聊名
int Group_view_admini_num(void *arg);
//退群线程
//调用之前需要给cm.tousername赋值为群名，cm.tocfd赋值为identity
void *Group_exit_group_chat(void *arg);
//调用之前需要给cm.tousername赋值为群名，cm.tocfd赋值为identity
void *Group_kick_member(void *arg);
//调用之前需要给cm.tousername赋值为群名,cm.tocfd赋值为identity
void *Group_Set_revoke_admini(void *arg);
//把消息发送给除了自己的所有群成员
//调用之前需要给cm.tousername赋值为群聊名称
//参数type:可取值3 4 5
void Group_broadcast(void *arg, char *q, int type);