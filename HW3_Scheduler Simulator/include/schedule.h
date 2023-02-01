#ifndef MY_SCHEDULE_H
#define MY_SCHEDULE_H
#include <ucontext.h>
#include <stdbool.h>
//variable
typedef struct schedule_t
{
    ucontext_t main;    //包含主程式的上下文
    int running_thread; //如果是-1，表示沒有執行中的thread
    int algorithm;      //0:FCFS, 1:RR, 2:PP
}schedule_t;

typedef struct node
{
    struct node * next;
    struct node * last;
    struct uthread_t * thread;
}NODE;


extern schedule_t * scheduler;

extern int timeunit;
extern bool interrupt;
extern bool CPUBusy;
extern int numOfWait;
extern int numOfReady;

extern NODE * readyQHead;
extern NODE * readyQTail;
extern NODE * waitQHead;
extern NODE * waitQTail;
extern NODE * runNode;

extern struct uthread_t * threads;
extern int created_thread_count;

//function

//interrupt routine
void iterrupt_routine_timer(int sig_num);
void iterrupt_routine_CtrlZ(int sig_num);

void time_unit_increase();

//init scheduler
void init_scheduler(int algo);
void init_timer();
void init_interrupt();
void stimulate();

//scheduler algorithm
void FCFS();
void RR();
void PP();

void addwaittime();
void addruntime();
void addreadytime();


//operation queue
void insertReadyQ(struct uthread_t * newthread);
void deleteReadyQ(NODE * current);
void insertWaitQ(struct uthread_t * newthread);
void deleteWaitQ(NODE * current);
void checkWaitQ();
#endif