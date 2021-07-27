#include "func.h"

char *get_time(char *now_time)
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	strcpy(now_time, asctime (timeinfo));
	now_time[strlen(now_time)-1] = '\0';

    return now_time;
}

int main()
{
    char now_time[BUFSIZ];

    get_time(now_time);

    printf("%s\n", now_time);
    

    return 0;
}