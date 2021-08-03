#include "func.h"
#include <libgen.h>

struct test
{
	int len;
	char name[BUFSIZ];
};
void my_err(const char *str, const int line)
{
    fprintf(stderr, "%d : %s : %s", line, str, strerror(errno));
    exit(1);
}
char *get_time(char now_time[BUFSIZ])
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	strcpy(now_time, asctime (timeinfo));
	now_time[strlen(now_time)-1] = '\0';

	char temp[BUFSIZ];
	strcpy(temp, now_time);
	// write(STDOUT_FILENO, temp, sizeof(temp));
}

MYSQL accept_mysql(void)
{
	MYSQL               mysql;
	
	if(NULL == mysql_init(&mysql)){
		my_err("mysql_init", __LINE__);
	}

	//初始化数据库
	if(mysql_library_init(0, NULL, NULL) != 0){
		my_err("mysql_library_init", __LINE__);
	}

	//连接数据库
	if(NULL == mysql_real_connect(&mysql, "127.0.0.1", "root", "xjmwsb1234", "testdb", 0, NULL, 0)){
		my_err("mysql_real_connect", __LINE__);
	}

	//设置中文字符集
	if(mysql_set_character_set(&mysql, "utf8") < 0){
		my_err("mysql_set_character_set", __LINE__);
	}
	
	printf("连接mysql数据库成功!\n");
	return mysql;
}

int main()
{
    accept_mysql();

    return 0;
}