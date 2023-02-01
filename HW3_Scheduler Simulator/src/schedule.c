
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include "../include/schedule.h"
#include "../include/uthread.h"
#include "../include/function.h"
#include "../include/resource.h"

struct itimerval interval;
struct sigaction act;
schedule_t * scheduler;

int numOfWait;
int numOfReady;
int timeunit;
bool interrupt;
bool interrupt_resume;
bool CPUBusy;
bool first_start;
bool simulate_over;
NODE * readyQHead;
NODE * readyQTail;
NODE * waitQHead;
NODE * waitQTail;
NODE * runNode;
int created_thread_count;

void init_scheduler(int algo)
{
    //init resource
	for(int i = 0; i < 8; ++i)
		resource_available[i] = true;

    freequeue    = (uthread_t *) malloc(sizeof(struct uthread_t) * MAX_UTHREAD_SIZE);
    created_thread_count = 0;
    numOfWait = 0;
    numOfReady = 0;
    scheduler    = (schedule_t *) malloc(sizeof(struct schedule_t));
    (*scheduler).running_thread = -1;
    (*scheduler).algorithm = algo;

    readyQHead   = NULL;
	readyQTail   = NULL;
	waitQHead = NULL;
	waitQTail = NULL;
    runNode   = NULL;
    interrupt = false; 
    interrupt_resume = false;
    CPUBusy = true;
}

void iterrupt_routine_CtrlZ(int sig_num)
{
    interrupt = true;
    interrupt_resume = true;
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTSTP, iterrupt_routine_CtrlZ);
    act.sa_handler = NULL;
    task_interrupt();
    return;
}
void checkIdle(){
    //if(readyQHead == NULL && waitQHead != NULL && runNode == NULL){
    if(numOfReady == 0 && numOfWait != 0 && runNode == NULL){
        if(CPUBusy == true ){
            printf("CPU idle.\n");
            fflush(stdout);
            CPUBusy = false;
        }
    }
    else{
        CPUBusy = true;
    }
}
void iterrupt_routine_timer(int sig_num)
{
	if(first_start){
		getitimer(ITIMER_VIRTUAL, &interval);
        first_start = !first_start;
        timeunit = 0;
	}
	else{
		timeunit ++;
        checkWaitQ();
        checkReadyQ();
        addwaittime();
        addreadytime();
        addruntime();
        if(scheduler->algorithm == 1 && timeunit%3 == 0){
            task_timeout();
        }
        checkIdle();
        if(readyQHead == NULL && waitQHead == NULL && runNode == NULL) simulate_over = true;
	}	
}
void init_interrupt()
{
    first_start = true;     
	signal(SIGTSTP, iterrupt_routine_CtrlZ); 
    interrupt = false;
}
void init_timer()
{
    act.sa_handler = iterrupt_routine_timer;
    interval.it_value.tv_sec = 0;
    interval.it_value.tv_usec = 1;
    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_usec = 10000;

    sigaction(SIGVTALRM, &act, NULL);
    setitimer(ITIMER_VIRTUAL, &interval, NULL);
    //reset timeunit
    timeunit = 0; 
}
void stimulate()
{
    simulate_over = false;
    printf("Start simulation.\n");
    fflush(stdout);
    init_interrupt(); //set interrupt 
	init_timer();     //set timer
    while((readyQHead != NULL || waitQHead != NULL) || (runNode != NULL)){
        if(simulate_over) break;
        if(interrupt) break;
    }
    if(interrupt==false){
        printf("Simulation over.\n");
        fflush(stdout);
    }
}
void checkReadyQ(){
    if(runNode != NULL && interrupt_resume == true){ // if there will be interrupt, it is need to be execute
        interrupt_resume= false;
        if(runNode->thread->state == RUNNING){
            CPUBusy = true;
            task_run();
        }
        else
            interrupt = false;
        if(!interrupt)
            runNode=NULL;
    }
    else if(readyQHead != NULL && runNode == NULL){
        runNode = readyQHead;
        deleteReadyQ(runNode);   //先離開ready queue
        CPUBusy = true;
        task_run();
        if(!interrupt)
            runNode=NULL;
    }
    else if(scheduler->algorithm == 2 && readyQHead != NULL && runNode != NULL){
        if(runNode->thread->priority > readyQHead->thread->priority){
            NODE * temp = runNode;
            runNode = readyQHead;
            deleteReadyQ(runNode);
            insertReadyQ(temp->thread);
            CPUBusy = true;
            printf("Task %s is running.\n",runNode->thread->name);
            fflush(stdout);
            swapcontext(&temp->thread->ctx, &runNode->thread->ctx);
        }
    }
}
void addwaittime(){
    if(waitQHead == NULL) return;
    NODE * current = waitQHead;
    do{
        NODE * next = current->next;
        struct uthread_t * currentWaitThread = (*current).thread;
        (*currentWaitThread).turnaround++;
        current = next;
    }while(current != waitQHead);
}
void addreadytime()
{
    if(readyQHead == NULL) return;
    NODE * current = readyQHead;
    do{
        NODE * next = current->next;
        struct uthread_t * currentReadyThread = current->thread;
        (*currentReadyThread).waittime++;
        (*currentReadyThread).turnaround++;
        current = next;
    }while(current != readyQHead);
}
void addruntime()
{
    if(runNode == NULL) return;
    uthread_t * current = runNode->thread;
    if(current->state == RUNNING){
        (*current).runtime++;
        (*current).turnaround++;
    }

}
void checkWaitQ()
{
    if(waitQHead == NULL) return;
    NODE * current = waitQHead;
    int wait_need_to_be_checked = numOfWait;
    while(wait_need_to_be_checked>0){
        NODE * next = current->next;
        uthread_t * currentWaitThread = (*current).thread;
        if((*currentWaitThread).waitcountdown != -1){ 
            (*currentWaitThread).waitcountdown--;
            if((*currentWaitThread).waitcountdown == 0){
                (*currentWaitThread).state = READY;
                (*currentWaitThread).waitcountdown = -1;
                insertReadyQ(currentWaitThread);
                deleteWaitQ(current);
            }
        }
        else{
            bool available = check_resources(currentWaitThread->count,currentWaitThread->resource_list);
            if(available){
                (*currentWaitThread).state = READY;
                insertReadyQ(currentWaitThread);
                deleteWaitQ(current);
            }
        }
        current = next;
        wait_need_to_be_checked--;
    }
}
/******* Ready Queue ***************************************/
void insertReadyQ(uthread_t * newThread){
    //insert node to double circular linked list
    NODE * newNode = (NODE *) malloc(sizeof(struct node));
    newNode->thread = newThread;
    if(numOfReady == 0){       //case : num of ready == 0
        newNode->next = newNode;
        newNode->last = newNode;
        readyQHead = newNode;
        readyQTail = newNode;
        numOfReady++;
        return;
    }
    else if(numOfReady == 1){          //case : num of ready == 1
        NODE * current = readyQHead;
        newNode->next = current;
        newNode->last = current;
        current->next = newNode;
        current->last = newNode;
        if((*newThread).priority < (*current).thread->priority){
            readyQHead = newNode;
            readyQTail = current;
        }
        else{
            readyQHead = current;
            readyQTail = newNode;
        }
        numOfReady++;
        return;
    }
    else{                            //case : num of ready > 1
        //search the position to insert
        NODE * current = readyQHead;
        do{
            if((*newThread).priority < (*current).thread->priority) break; //代表要插在current前面
            current = current->next;
        }while(current != readyQHead);
        if(current == readyQTail){
            if((*newThread).priority >= (*current).thread->priority){ //insert to the last node
                //insert after current
                current->next = newNode;
                newNode->last = current;
                newNode->next = readyQHead;
                readyQHead->last = newNode;
                readyQTail = newNode;
                numOfReady++;
                return;
            }
        }
        //insert before current
        newNode->next = current;
        newNode->last = current->last;
        current->last->next = newNode;
        current->last = newNode;
        if(current == readyQHead){
            if((*newThread).priority < (*current).thread->priority){
                readyQHead = newNode;
                readyQTail = newNode->last;
            }
            else{
                readyQTail = newNode;
            }
        }
        numOfReady++;
        return;
    }
}

void deleteReadyQ(NODE * current){
    numOfReady--;
    //delete node from double circular linked list
    if(readyQHead == readyQTail){ // only one node
        readyQHead = NULL;
        readyQTail = NULL;
    }
    else{
        if(current == readyQHead){
            readyQHead = (*current).next;
            readyQHead->last = readyQTail;
            readyQTail->next = readyQHead;
        }
        else if(current == readyQTail){
            readyQTail = (*current).last;
            readyQTail->next = readyQHead;
            readyQHead->last = readyQTail;
        }
        else{
            (*(*current).last).next = (*current).next;
            (*(*current).next).last = (*current).last;
        }
    }
}
/******* Ready Queue ***************************************/
/******* Wait Queue  ***************************************/
void insertWaitQ(uthread_t * newThread){
    numOfWait++;
    //insert node to double circular linked list
    NODE * newNode = (NODE *) malloc(sizeof(struct node));
    newNode->thread = newThread;
    if(waitQTail == NULL){
        newNode->next = newNode;
        newNode->last = newNode;
        waitQHead = newNode;
        waitQTail = newNode;
    }
    else{
        newNode->next = waitQHead;
        waitQHead->last = newNode;
        newNode->last = waitQTail;
        waitQTail->next = newNode;
        waitQTail = newNode;
    }
}
void deleteWaitQ(NODE * current){
    numOfWait--;
    //delete node from double circular linked list
    if(waitQHead == waitQTail){ // only one node
        waitQHead = NULL;
        waitQTail = NULL;
    }
    else{                //delete more than one node
        if(current == waitQHead){
            waitQHead = (*current).next;
            waitQHead->last = waitQTail;
            waitQTail->next = waitQHead;
        }
        else if(current == waitQTail){
            waitQTail = (*current).last;
            waitQTail->next = waitQHead;
            waitQHead->last = waitQTail;
        }
        else{
            (*(*current).last).next = (*current).next;
            (*(*current).next).last = (*current).last;
        }
    }
}
/******* Wait Queue ***************************************/
