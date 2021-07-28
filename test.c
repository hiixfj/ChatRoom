#include "func.h"

char *get_time(char now_time[BUFSIZ])
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	strcpy(now_time, asctime (timeinfo));
	now_time[strlen(now_time)-1] = '\0';

	char temp[BUFSIZ];
	// memset(temp, 0, sizeof(temp));
	strcpy(temp, now_time);
	write(STDOUT_FILENO, temp, sizeof(temp));
}

int main()
{
    char now_time[BUFSIZ];

    get_time(now_time);

    // printf("%s\n", now_time);
    

    return 0;
}