#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "include/shell.h"
#include "include/command.h"
#include "include/schedule.h"
struct itimerval interval;
struct sigaction act;

char *algo_name[3] = {"FCFS", "RR", "PP"};
int algorithm;

int main(int argc, char *argv[])
{
	fflush(stdin);
	fflush(stdout);
	for(int i = 0; i < 3; ++i){
		if(strcmp(argv[1],algo_name[i])==0){
			algorithm = i;
			break;
		}
	}
	init_scheduler(algorithm);

	history_count = 0;
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
    	history[i] = (char *)malloc(BUF_SIZE * sizeof(char));

	shell();

	for (int i = 0; i < MAX_RECORD_NUM; ++i)
    	free(history[i]);

	return 0;
}
