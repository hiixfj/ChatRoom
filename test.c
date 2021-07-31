#include "func.h"
#include <libgen.h>

struct test
{
	int len;
	char name[BUFSIZ];
};

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

int main()
{
    char buf[BUFSIZ];
	int temp = 111;

	sprintf(buf, "%d", temp);

	printf("%s\n", buf);

    return 0;
}