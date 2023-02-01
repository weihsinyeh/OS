#include "../include/resource.h"
#include "../include/schedule.h"
#include "../include/uthread.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
bool resource_available[8];
void get_resources(int count, int *resources)
{
    uthread_t *current = runNode->thread;
    current->old_count = current->count;
    current->old_resource_list = current->resource_list;
    current->count = count;
    current->resource_list = resources;
    bool available;
    do{
        available = check_resources(count, resources);
        if(!available){
            task_wait();
            available = check_resources(count, resources);
        }
    }while(!available);
    for(int i = 0; i < count; ++i){
        printf("Task %s gets resource %d.\n",runNode->thread->name,resources[i]);
        fflush(stdout);
        resource_available[resources[i]]=false;
    }
    current->old_count += current->count;
    current->old_resource_list = realloc(current->old_resource_list, sizeof(int) * current->old_count);
    for(int i = 0; i < current->count; ++i){
        current->old_resource_list[current->old_count - current->count + i] = current->resource_list[i];
    }
    current->count = current->old_count;
    current->resource_list = current->old_resource_list;
    return;
}
bool check_resources(int count, int *resources)
{
    for(int i = 0; i < count; ++i){
        if(resource_available[resources[i]]==false){
            return false;
        }
    }
    return true;
}
void release_resources(int count, int *resources)
{
    for(int i = 0; i < count; ++i){
        printf("Task %s releases resource %d.\n",runNode->thread->name,resources[i]);
        fflush(stdout);
        resource_available[resources[i]] = true;
    }
    uthread_t *current = runNode->thread;
    current->count = 0;
    current->resource_list = NULL;
    current->old_count = 0;
    current->old_resource_list = NULL;
}
