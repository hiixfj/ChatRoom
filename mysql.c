#include "func.h"

int mysql_repeat(MYSQL *mysql, const char *string, const char *str, int field)
{
    int rows;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char query_str[BUFSIZ];

    sprintf(query_str, "select * from %s", string);
    rows = mysql_real_query(mysql, query_str, strlen(query_str));
    if(rows != 0)
    {
        my_err("mysql_real_query error", __LINE__);
    }
    res = mysql_store_result(mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        if(strcmp(row[field - 1], str) == 0)
        {
            return 0;
        }
    }

    return 1;
}

int mysql_inquire_newsnum(MYSQL *mysql, const char *username, int line)
{
    int rows;
    MYSQL_ROW row, row2;
    MYSQL_RES *res, *res2;

    int newsnum = 0;
    char query_str[BUFSIZ];

    memset(query_str, 0, sizeof(query_str));
    sprintf(query_str, "select newsnum from UserData where username = \"%s\"", username);
    rows = mysql_real_query(mysql, query_str, strlen(query_str));
    if(rows != 0)
    {
        printf("%d:error:%s\n", line, mysql_error(mysql));
        my_err("mysql_real_query error", __LINE__);
    }
    res = mysql_store_result(mysql);
    if(res == NULL)
    {
        printf("%d:error:%s\n", line, mysql_error(mysql));
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
        newsnum = atoi(row[0]);
    
    //plus补全版
    newsnum = 0;

    sprintf(query_str, "select * from OffLineMes where touser = \"%s\"", username);
    MY_real_query(mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        newsnum++;
    }

    //plus pro超级补全版
    //补全了自己作为群管理或者群主应该受到的newsnum
    sprintf(query_str, "select * from %s where num = \"3\" or num = \"4\"", username);
    MY_real_query(mysql, query_str, strlen(query_str), __LINE__);
    res = mysql_store_result(mysql);
    if(res == NULL)
    {
        my_err("mysql_store_result error", __LINE__);
    }
    while(row = mysql_fetch_row(res))
    {
        sprintf(query_str, "select * from OffLineMes where touser = \"%s\"", row[0]);
        MY_real_query(mysql, query_str, strlen(query_str), __LINE__);
        res2 = mysql_store_result(mysql);
        if(res2 == NULL)
        {
            my_err("mysql_store_result error", __LINE__);
        }
        while(row2 = mysql_fetch_row(res2))
        {
            newsnum++;
        }
    }
    

    return newsnum;
}

int MY_real_query(MYSQL *mysql, const char *q, unsigned long length, int line)
{
    int rows;
    MYSQL_ROW row;
    MYSQL_RES *res;

    rows = mysql_real_query(mysql, q, length);
    if(rows != 0)
    {
        printf("%d:error:%s\n", line, mysql_error(mysql));
        my_err("mysql_real_query error", line);
    }

    return 0;
}