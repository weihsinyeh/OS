#include <ucontext.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include "../include/task.h"
#include "../include/uthread.h"
#include "../include/schedule.h"
#include "../include/function.h"

void task_sleep(int ms) //進入waiting state 
{
    uthread_t *t = runNode->thread;
    t->state = WAITING;                 //set state
    t->waitcountdown = ms;              //過了幾秒後會被喚醒
    (*scheduler).running_thread = -1;
    printf("Task %s goes to sleep.\n",t->name);
    fflush(stdout);
    insertWaitQ(t);             //insert waiting queue;
    runNode = NULL;
    swapcontext(&t->ctx,&((*scheduler).main));
}

void task_wait()
{
    uthread_t *t = runNode->thread;
    t->state = WAITING;                 //set state
    printf("Task %s is waiting resource.\n",t->name);
    fflush(stdout);
    t->waitcountdown = -1;
    (*scheduler).running_thread = -1;
    insertWaitQ(t);             //insert waiting queue;
    runNode = NULL;
    swapcontext(&t->ctx,&((*scheduler).main));
}
void task_exit() //離開running state
{
    uthread_t *t = runNode->thread;
    runNode = NULL;
    t->state = TERMINATED;              //set state
    (*scheduler).running_thread = -1;
    printf("Task %s has terminated.\n",t->name);
    fflush(stdout);
    swapcontext(&(t->ctx),&((*scheduler).main));
}
void task_interrupt() //還在running的狀態下，被ctrl+z
{
    if(runNode == NULL) return;
    uthread_t *t = runNode->thread;
    t->state = RUNNING;                 //set state
    (*scheduler).running_thread = t->id;
    swapcontext(&(t->ctx),&((*scheduler).main));
}
void task_run()
{
    if(runNode == NULL) return;
    uthread_t *t = runNode->thread;
    (*scheduler).running_thread = t->id;
    t->state = RUNNING;                         //set state
    printf("Task %s is running.\n",t->name);
    fflush(stdout);
    swapcontext(&((*scheduler).main),&(t->ctx));   
}
void task_timeout()
{
    if(runNode == NULL) return;
    uthread_t *t = runNode->thread;
    (*scheduler).running_thread = t->id;
    if(readyQHead!=NULL){
        t->state = READY;   
        runNode = readyQHead;
        uthread_t *next_t = runNode->thread;
        next_t->state = RUNNING;
        deleteReadyQ(runNode);
        insertReadyQ(t);
        printf("Task %s is running.\n",next_t->name);
        fflush(stdout);
        swapcontext(&(t->ctx),&(next_t->ctx));
    }
}