#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#include "../include/function.h"
#include "../include/builtin.h"
#include "../include/command.h"
#include "../include/uthread.h"
#include "../include/schedule.h"

char * statename[4] ={ "READY", "WAITING", "RUNNING", "TERMINATED" };

int help(char **args)
{
	int i;
    printf("--------------------------------------------------\n");
  	printf("My Little Shell!!\n");
	printf("The following are built in:\n");
	for (i = 0; i < num_builtins(); i++) {
    	printf("%d: %s\n", i, builtin_str[i]);
  	}
	printf("%d: replay\n", i);
    printf("--------------------------------------------------\n");
	return 1;
}

int cd(char **args)
{
	if (args[1] == NULL) {
    	fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  	} else {
    	if (chdir(args[1]) != 0)
      		perror("lsh");
	}
	return 1;
}

int echo(char **args)
{
	bool newline = true;
	for (int i = 1; args[i]; ++i) {
		if (i == 1 && strcmp(args[i], "-n") == 0) {
			newline = false;
			continue;
		}
		printf("%s", args[i]);
		if (args[i + 1])
			printf(" ");
	}
	if (newline)
		printf("\n");

	return 1;
}

int exit_shell(char **args)
{
	return 0;
}

int record(char **args)
{
	if (history_count < MAX_RECORD_NUM) {
		for (int i = 0; i < history_count; ++i)
			printf("%2d: %s\n", i + 1, history[i]);
	} else {
		for (int i = history_count % MAX_RECORD_NUM; i < history_count % MAX_RECORD_NUM + MAX_RECORD_NUM; ++i)
			printf("%2d: %s\n", i - history_count % MAX_RECORD_NUM + 1, history[i % MAX_RECORD_NUM]);
	}
	return 1;
}

bool isnum(char *str)
{
	for (int i = 0; i < strlen(str); ++i) {
    	if(str[i] >= 48 && str[i] <= 57)
			continue;
        else
		    return false;
  	}
  	return true;
}

int mypid(char **args)
{
	char fname[BUF_SIZE];
	char buffer[BUF_SIZE];
	if(strcmp(args[1], "-i") == 0) {

	    pid_t pid = getpid();
	    printf("%d\n", pid);
	
	} else if (strcmp(args[1], "-p") == 0) {
	
		if (args[2] == NULL) {
      		printf("mypid -p: too few argument\n");
      		return 1;
    	}

    	sprintf(fname, "/proc/%s/stat", args[2]);
    	int fd = open(fname, O_RDONLY);
    	if(fd == -1) {
      		printf("mypid -p: process id not exist\n");
     		return 1;
    	}

    	read(fd, buffer, BUF_SIZE);
	    strtok(buffer, " ");
    	strtok(NULL, " ");
	    strtok(NULL, " ");
    	char *s_ppid = strtok(NULL, " ");
	    int ppid = strtol(s_ppid, NULL, 10);
    	printf("%d\n", ppid);
	    
		close(fd);

  	} else if (strcmp(args[1], "-c") == 0) {

		if (args[2] == NULL) {
      		printf("mypid -c: too few argument\n");
      		return 1;
    	}

    	DIR *dirp;
    	if ((dirp = opendir("/proc/")) == NULL){
      		printf("open directory error!\n");
      		return 1;
    	}

    	struct dirent *direntp;
    	while ((direntp = readdir(dirp)) != NULL) {
      		if (!isnum(direntp->d_name)) {
        		continue;
      		} else {
        		sprintf(fname, "/proc/%s/stat", direntp->d_name);
		        int fd = open(fname, O_RDONLY);
        		if (fd == -1) {
          			printf("mypid -p: process id not exist\n");
          			return 1;
        		}

        		read(fd, buffer, BUF_SIZE);
        		strtok(buffer, " ");
        		strtok(NULL, " ");
        		strtok(NULL, " ");
		        char *s_ppid = strtok(NULL, " ");
		        if(strcmp(s_ppid, args[2]) == 0)
		            printf("%s\n", direntp->d_name);

        		close(fd);
     		}
	   	}	
		closedir(dirp);
	} 
	else {
    	printf("wrong type! Please type again!\n");
  	}
	
	return 1;
}

int add(char **args) //add {task_name} {function_name} {priority}
{
	char *task_name = (char*) malloc(sizeof(char) * strlen(args[1]));
	strcpy(task_name, args[1]);

	int priority = 0;  // default priority is 0
	if(scheduler->algorithm == 2) // algorithm is PP
		priority = atoi(args[3]);
	
	bool is_task = false;
	for (int i = 0; i < num_tasks(); ++i){
		if (strcmp(args[2], task_str[i]) == 0){
			uthread_create(task_name, task_func[i], priority);	
			is_task = true;
			break;
		}
	}	
	if(!is_task){
		if(strcmp(args[2],"test_exit")==0)
			uthread_create(task_name, &test_exit, priority);	
		else if(strcmp(args[2],"test_sleep")==0)
			uthread_create(task_name, &test_sleep, priority);	
		else if(strcmp(args[2],"test_resource1")==0)
			uthread_create(task_name, &test_resource1, priority);
		else if(strcmp(args[2],"test_resource2")==0)
			uthread_create(task_name, &test_resource2, priority);
		else if(strcmp(args[2],"idle")==0)
			uthread_create(task_name, &idle, priority);
		else{
			printf("Task %s is not found.\n",args[2]);
			return 1;
		}
	}
	//fflush(stdout);
	printf("Task %s is ready.\n", task_name);
	fflush(stdout);
	return 1;
}

int del(char **args)
{
	char *task_name = (char*) malloc(sizeof(char) * strlen(args[1]));
	strcpy(task_name, args[1]);
	uthread_delete(task_name);	
	printf("Task %s is killed.\n",args[1]);
	return 1;
}

int ps(char **args)
{
	printf("\n");
	printf("%5s|%8s|%12s|%10s|%10s|%10s|%16s","TID","name","state","running","waiting","turnaround","resources");
	if(scheduler->algorithm==2)
		printf("|%10s","priority");
	printf("\n");
	printf("-------------------------------------------------------------------------------------------\n");
	for(int i=0;i<created_thread_count;i++){
		printf("%5d|",i+1);
		printf("%8s|",((freequeue[i]).name));
		printf("%12s|",statename[(freequeue[i]).state]);
		printf("%10d|",((freequeue[i]).runtime));
		printf("%10d|",((freequeue[i]).waittime));
		if((freequeue[i]).state == 3)
			printf("%10d|",((freequeue[i]).turnaround));
		else 
			printf("%10s|","none");
			
		if((freequeue[i]).old_count == 0)
			printf("%16s","none");
			
		else{
			for(int j=0;j<(8-(freequeue[i]).old_count);j++)
				printf("%2s"," ");
			for(int j=0;j<(freequeue[i]).count;j++)
				printf("%2d",((freequeue[i]).old_resource_list[j]));
		}
		if(scheduler->algorithm == 2)
			printf("|%10d",((freequeue[i]).priority));
		printf("\n");
	}
	return 1;
}

int start(char **args)
{
	stimulate();
	return 1;
}

const char *builtin_str[] = {
 	"help",
 	"cd",
	"echo",
 	"exit",
 	"record",
	"mypid",
	"add",
	"del",
	"ps",
	"start",
};

const int (*builtin_func[]) (char **) = {
	&help,
	&cd,
	&echo,
	&exit_shell,
  	&record,
	&mypid,
	&add,
	&del,
	&ps,
	&start,
};

int num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

const char *task_str[] = {
 	"task1",
	"task2",
 	"task3",
 	"task4",
	"task5",
	"task6",
	"task7",
	"task8",
	"task9",
};

const int (*task_func[]) (char **) = {
	&task1,
	&task2,
	&task3,
	&task4,
	&task5,
	&task6,
	&task7,
	&task8,
	&task9,
};

int num_tasks() {
	return sizeof(task_str) / sizeof(char *);
}