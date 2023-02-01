#include <stdio.h>
#include <stddef.h>
#include <ucontext.h>
#include "../include/resource.h"
#include "../include/uthread.h"
#include "../include/schedule.h"


struct uthread_t * freequeue;

void uthread_create(char * task_name,Fun task_func, int priority)
{
    uthread_t *t = &(freequeue[created_thread_count]);
    t->id = created_thread_count++;
    t->state = READY;
    t->func = task_func;
    t->name = task_name;
    t->priority = priority;
    getcontext(&(t->ctx));

    t->ctx.uc_stack.ss_sp = t->stack;
    t->ctx.uc_stack.ss_size = DEFAULT_STACK_SZIE;
    t->ctx.uc_stack.ss_flags = 0;
    t->ctx.uc_link = &((*scheduler).main);

    //set time
    t->waittime = 0;
    t->runtime = 0;
    t->turnaround = 0;
    t->waitcountdown = 0;
    
    t->count = 0;
    t->resource_list = NULL;
    t->old_count = 0;
    t->old_resource_list = NULL;
    //double circular linked list insert
    insertReadyQ(t);
    makecontext(&(t->ctx),t->func ,1,scheduler);
}
void uthread_delete(char * task_name)
{
    for(int i=0;i<created_thread_count;i++){
        if(strcmp(freequeue[i].name,task_name)==0){
            if(freequeue[i].state == RUNNING){
                scheduler->running_thread = -1;
                freequeue[i].state = TERMINATED;

                for(int i = 0; i < freequeue[i].old_count; ++i){
                    resource_available[freequeue[i].old_resource_list[i]] = true;
                }
                freequeue[i].count = 0;
                freequeue[i].resource_list = NULL;
                freequeue[i].old_count = 0;
                freequeue[i].old_resource_list = NULL;
                break;
            }
            if(freequeue[i].state == WAITING){ 
                freequeue[i].state = TERMINATED;
                NODE* current = waitQHead;
                while(current != NULL){
                    if(current->thread->id == freequeue[i].id){
                        for(int i = 0; i < current->thread->old_count; ++i){
                            resource_available[current->thread->old_resource_list[i]] = true;
                        }
                        current->thread->count = 0;
                        current->thread->resource_list = NULL;
                        current->thread->old_count = 0;
                        current->thread->old_resource_list = NULL;
                        deleteWaitQ(current);
                        break;
                    }
                    current = current->next;
                }
            }
            if(freequeue[i].state == READY){
                freequeue[i].state = TERMINATED;
                NODE* current = readyQHead;
                NODE* next = current->next;
                do{
                    next = current->next;
                    if(current->thread->id == freequeue[i].id){
                        for(int i = 0; i < current->thread->old_count; ++i){
                            resource_available[current->thread->old_resource_list[i]] = true;
                        }
                        current->thread->count = 0;
                        current->thread->resource_list = NULL;
                        current->thread->old_count = 0;
                        current->thread->old_resource_list = NULL;
                        deleteReadyQ(current);
                        break;
                    }
                    current = current->next;
                }while(current != readyQHead);
            }
            break;
        }
    }
}
