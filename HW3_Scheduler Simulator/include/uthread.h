#ifndef MY_UTHREAD_H
#define MY_UTHREAD_H
#include <ucontext.h>
#define DEFAULT_STACK_SZIE (1024*128)
#define MAX_UTHREAD_SIZE   1024
extern struct uthread_t * freequeue;
extern enum ThreadState{READY,WAITING,RUNNING,TERMINATED};

typedef void (*Fun)(void *);

typedef struct uthread_t
{
    int id;
    enum ThreadState state;
    Fun func;
    char * name;
    int priority;

    ucontext_t ctx;
    char stack[DEFAULT_STACK_SZIE];

    int waittime;
    int runtime;
    int turnaround;
    int waitcountdown;

    int count;
    int * resource_list;
    int old_count;
    int * old_resource_list;
}uthread_t;


//void uthread_resume();
void uthread_create(char * task_name,Fun task_func, int priority);
void uthread_delete(char * task_name);
#endif
