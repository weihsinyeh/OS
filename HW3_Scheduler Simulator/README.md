# HW3
###### tags: `OS`
```c=
void init_timer()
{
    act.sa_handler = iterrupt_routine_timer;
    interval.it_value.tv_sec = 0;
    interval.it_value.tv_usec = 1;
    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_usec = 10000;

    sigaction(SIGVTALRM, &act, NULL);
    setitimer(ITIMER_VIRTUAL, &interval, NULL);
```

set timer
value = sec 0 (0秒後才開始)

value = usec 1 (1微秒才開始)

interval = sec 0 (兼距0)



![](https://i.imgur.com/GE8e2Fv.png)
![](https://i.imgur.com/subeFbT.png)


| Column 1 | 我 |CPU idle time| 楊 |CPU idle time|
| -------- | -------- | -------- | -------- | -------- |
| test_case1_FCFS    | V     | 3     | V     | Text     |
| test_case1_RR      | Text     | 1     | Text     | Text     |
| test_case1_PP      | V     | 0     | V     | Text     |
| test_case2_FCFS    | V     | 2     | V     | Text     |
| test_case2_RR      | V     | 2     | V     | Text     |
| test_case2_PP      | V     | 2     | V     | Text     |
| general_FCFS       | V     | 2     | V     | Text     |
| general_RR         | V     | 1     | V     | Text     |
| general_PP         | V     | 1     | V     | Text     |

## 一.讓程式run起來
* 編譯模擬環境: 在makefile的層級下，輸入
```
make
```
* 執行模擬環境: 
```
./scheduler_simulator
```
## 二.撰寫程式碼順序
1. builtin.c 寫 add() : ucontext
>  四種函數介紹 : https://www.796t.com/content/1546497925.html
```c=
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

int main(int argc, const char *argv[]){
    ucontext_t context;

    getcontext(&context); //程式通過getcontext先儲存了一個上下文
    puts("Hello world");  //,然後輸出"Hello world",
    sleep(1);
    setcontext(&context); //在通過setcontext恢復到getcontext的地方，重新執行程式碼
    return 0;
}
```
output:
:::success
[email protected]:~$ ./example 
Hello world
Hello world
Hello world
Hello world
Hello world
Hello world
Hello world
^C
[email protected]:~$
:::


>  https://pubs.opengroup.org/onlinepubs/7808799/xsh/ucontext.h.html
>  https://man7.org/linux/man-pages/man3/getcontext.3.html
* 如何利用ucontext 串出multi-threads的效果
* https://github.com/Winnerhust/uthread

## Requirement

1. Tasks & task manager
    * Use context and the related APIs to create tasks
    * Each task runs a function 定義在 function.c
    * Task manager 應該為一個資料結構 struct 
        * 包含 state, data structures 

4. Timer & signal handler
    * use related system calls to set a timer that should send a signal (SIGVTALRM) every 10ms
    * The signal handler should do the followings:
        * Calculate all task-related time(granularity : 10ms)
        * Check if any task's state 
        * Decide whether re-scheduling is needed
    作法 
        用 sustem call 去設定10ms
        [linux_timer](https://wirelessr.gitbooks.io/working-life/content/linux_timer.html)
        [jserv](https://hackmd.io/-jPjG8riTy27CsAMM45Qnw?view)
          
    setitime這個API是alarm的進化版
    ```c=1
    int getitimer(int which, struct itimerval *curr_value);
    ```
    那個which可以決定今天要發哪個signal，話是這麼說，但也只有三種選擇
    :::info
        ITIMER_REAL -> SIGALRM
        ITIMER_VIRTUAL -> SIGVTALRM
        ITIMER_PROF -> SIGPROF 
    :::
    至少可以三個timer同時run了，而取消的辦法則是setitimer重call一次且把時間設成0，勉勉強強，還算OK，如果趕時間，這樣做也無可厚非啦，不過，用setitimer還不如用timer_create。

    timer_create這個API就是一和二的集大成了

    ```c=1
    int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
    ```
    這API的參數就豐富了，clockid可以選CPU的時脈，看是單純count user space還是kernel space或total，也可以自帶signal和signal handler進去，能用的signal是signal RT系列的，從 SIGRTMIN~ SIGRTMAX。

    簡單放一下用法，首先先把signal handler裝起來
    ```c=1
    void install_handler(int sig, void (*handle)(int s))
    {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = handler;
        sigemptyset(&sa.sa_mask);
        if(sigaction(sig, &sa, NULL) == -1)
            perror("sigaction");
    }
    ```
    接著把timer的時間設定好，記得return的timer要存起來
    ```c=1
    timer_t create_timer(int sig, int sec)
    {
        timer_t timerid;
        struct sigevent sev;
        struct itimerspec its;

        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = sig;
        sev.sigev_value.sival_ptr = &timerid;
        if(timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
            perror("timer_create");

        its.it_value.tv_sec = sec;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;

        if(timer_settime(timerid, 0, &its, NULL) == -1)
            perror("timer_settime");
        return timerid;
    }
    ```
    之後如果要取消，就可以直接用timer_delete砍掉，這種型態的timer已經稱的上是signal型態的完全體了。非常容易實做，code也容易復用，更重要的是，絕對精準，且完全沒有overhead，唯一的缺點是，signal handler絕對要注意reentrance的限制，例如：千萬不要在signal handler內做IO或mutex，不然自己的code怎麼dead lock的你都不會知道。
    https://blog.csdn.net/fjt19900921/article/details/8072903
```c=1
int add(char **args)
{
	char stack[1024*128];
    ucontext_t uc1,ucmain;
    getcontext(&uc1);
    uc1.uc_stack.ss_sp = stack;
    uc1.uc_stack.ss_size = 1024*128;
    uc1.uc_stack.ss_flags = 0;
    uc1.uc_link = &ucmain;
    printf("before enter task\n");
	for (int i = 0; i < num_tasks(); ++i){
		if (strcmp(args[1], task_str[i]) == 0){
			//makecontext(&uc1,task_func[i],0); //<- 實際用
			makecontext(&uc1,task_func[9],0);  //<- 測試用
			swapcontext(&ucmain,&uc1);	
			printf("return to main\n");
			break;
		}
	}	
	return 1;
}
```
[Ctrl+z](https://www.geeksforgeeks.org/c-program-not-suspend-ctrlz-pressed/)

[Implementing a Job Control Shell](https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html)

[The GNU C Library](https://www.gnu.org/software/libc/manual/html_node/index.html)

[Job Control](28 Job Control)

[Compile C](https://www.geeksforgeeks.org/compiling-a-c-program-behind-the-scenes/)

[ctrl + z VS ctrl + c](https://superuser.com/questions/262942/whats-different-between-ctrlz-and-ctrlc-in-unix-command-line)

[Linux中的定時器:alarm()及setitimer()](https://blog.xuite.net/lidj37/twblog/179517551)
```c=1
void uthread_resume(int id)
{
    id = 0;
    if(id < 0 || id >= (*scheduler).max_index_of_free){
        return;
    }
    uthread_t *t = &((*scheduler).free_threads[id]);

    if (t->state == READY) {                    //ready 就直接切換到該context 繼續執行
        makecontext(&(t->ctx),t->func ,1,scheduler);
        swapcontext(&((*scheduler).main),&t->ctx);
        printf("task_name %s",t->name);        //swapcontext(&((*scheduler).main),&(t->ctx));
    }
}

void uthread_yield() //將自己的context保存起來，切換到main context
{
    if((*scheduler).running_thread != -1 ){
        uthread_t *t = &((*scheduler).free_threads[(*scheduler).running_thread]);
        t->state = READY;
        (*scheduler).running_thread = -1;

        swapcontext(&(t->ctx),&((*scheduler).main));
    }
}
/*
void uthread_body(schedule_t *ps)
{
    int id = ps->running_thread;
    if(id != -1){
        uthread_t *t = &((*ps).threads[id]);
        t->func(t->arg);
        t->state = FREE;
        ps->running_thread = -1;
    }
}*/

```
[林彥廷提供](https://nitish712.blogspot.com/2012/10/thread-library-using-context-switching.html?fbclid=IwAR39pQqR3cZTEMqKLzUE5SnJMoNNbEL9UGFbfrsSKiaVfJ5NbbhxkxKsGa0)
## 問題
1. ctrl + z 沒有辦法有第二次
:::success
ctrl + z 之後要用resume
好像有沒有再做signal(SIGTSTP,interrupt_routine_CtrlZ); 都沒差
:::
3. CPU idle次數
:::success
![](https://i.imgur.com/YHjwVIL.png)
:::
3. auto 的out檔
:::success
![](https://i.imgur.com/Xn2UHUG.png)
![](https://i.imgur.com/GN5fdmX.png)
:::
4. get resource 列印順序
::: success
![](https://i.imgur.com/7KOAMHt.png)
:::
5. get resource 跟 occupy resource 順序
6. CPU idle次數不同
:::success
![](https://i.imgur.com/XLfunJc.png)
:::
:::info
![](https://i.imgur.com/p7Z2rbY.png)

:::
7. turn around
[Turn Around time = Burst time + Waiting time](https://www.gatevidyalay.com/turn-around-time-response-time-waiting-time/)
[筆記](https://zys-notes.blogspot.com/2020/10/blog-post_15.html)
![](https://i.imgur.com/lrW1MEf.png)
![](https://i.imgur.com/QTx6Pzf.png)
![](https://i.imgur.com/2z3OmHe.png)
8. 第二次問題 我還要改INTERRUPT
:::success
![](https://i.imgur.com/OHWb3qn.png)
:::
:::info
![](https://i.imgur.com/Xhmy52t.png)
:::

9. JF 的問題

![](https://i.imgur.com/wmUTkqg.png)

![](https://i.imgur.com/8i7mBV6.png)

![](https://i.imgur.com/X6WHaU5.png)

![](https://i.imgur.com/NhfEoGx.png)

![](https://i.imgur.com/yk31GYg.png)

debug 方式:
時間差是為了找錯誤那行 報錯print的關係
你看是哪個task的名字開始錯 當名字是它的時候才印 印一行就好 印的出來再換其他行試試看，一次測一行印東西 之後就會慢慢鎖定範圍 範圍越來越小 就發現 阿原來這行不小心打了
get resource 附近那裡有問題 或release resources
因為我禮拜五是要調整resources 的時候有出現這件事情
阿禮拜六發現是 double linked list insert 有問題
怕 list 壞掉 你可以印ready queue 的數量 跟 wait queue的數量

加一個if 誰null 就直接return
可能進去的時候確定有給runnng thread Id 值
有swap前都檢查一下

10. resource兩階段

[realloc](https://blog.gtwang.org/programming/c-memory-functions-malloc-free/)

11. CPU idle

![](https://i.imgur.com/sQoF2ZN.png)
12. auto 檔案的問題
test_case1/2 start 與 over 包含這兩個 要導入到 out 
general 則不會
要做到就每個printf 後 加 fflush(out)
## 測資
check whether the shell is broken due to your modification
```
python3 test/judge_shell.py
```
**Auto-run the scheduler simulator**
* python3 test/auto_run.py {algorithm} {test_case}

* algorithm can be FCFS, RR, PP or all, where all will perform three rounds of simulation for all

**scheduling algorithms**
* test_case can be test/general.txt, test/test_case1.txt or test/test_case2.txt

    * Test case files contain a list of commands seperated by newlines. You can also write your own test cases, just follow the format
    
    * test/general.txt tests all requirements except pausing
    
    * test/test_case1.txt and test/test_case2.txt are test cases that need to observe the results
* The auto-run script will generate a file to store the simulation result, and the file name is {test_case'sfile name}_{algorithm}.txt, for example, general_FCFS.txt
* Auto-run script does not support pausing with Ctrl + z
```
python3 test/auto_run.py FCFS test/test_case1.txt
```
```c=
void insertReadyQ(uthread_t * newThread){
    numOfReady++;
    //insert node to double circular linked list
    NODE * newNode = (NODE *) malloc(sizeof(struct node));
    newNode->thread = newThread;
    if(readyQTail == NULL){
        newNode->next = newNode;
        newNode->last = newNode;
        readyQHead = newNode;
        readyQTail = newNode;
    }
    else{
        //fibd the position to insert
        NODE * current = readyQHead;
        int ready_need_to_be_checked = numOfReady;
        while(ready_need_to_be_checked>1){
            NODE * next = current->next;
            if((*current).thread->priority < (*newThread).priority){
                break;
            }
            current = next;
            ready_need_to_be_checked--;
        }
        //insert
        newNode->next = current;
        newNode->last = current->last;
        current->last->next = newNode;
        current->last = newNode;
    }
}
```
```c=
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
```
```c=
void task_timeout()
{
    if(runNode == NULL) return;
    uthread_t *t = runNode->thread;
    (*scheduler).running_thread = t->id;
    if(readyQHead!=NULL){
        t->state = READY;   
        runNode = readyQHead;
        uthread_t *next_t = runNode->thread;
        deleteReadyQ(runNode);
        insertReadyQ(t);
        swapcontext(&(t->ctx),&(next_t->ctx));
    }
}
```
[我](https://drive.google.com/drive/folders/1klCd2vfaT3qZA88A5SWCd9PhmTOcnArT)

[signal 我](https://l.facebook.com/l.php?u=https%3A%2F%2Fdrive.google.com%2Fdrive%2Ffolders%2F1Wur3jvRJFIVrgTZ55lhaAmrJ907M0zSd%3Fusp%3Dshare_link%26fbclid%3DIwAR3f8hR_u-OnA_29DbV4xQ4kcau4TqIhAdZjOV9gNG70JESDUnxOKOggohE&h=AT1ozz8Eh3njQlobCa2roj7gI_Q796pZyaAcq1bqm8w089WcREx1bRE3kazxYkiKWQiaYVMXx7-wR_UVF4T1PI_6vKmKQjKyx6DscDCgNlRWJqQcjEsGdGULWoOo-1Nafc-aaQ)

一種可以解釋說 handler剛開始就先把waiting的都丟到ready 然後reschedule 這樣就會少CPU idle
另一種解釋就是 handler負責把waiting的都丟到ready 然後等下個signal再reschedule，這樣就會多一個CPU idle
![](https://i.imgur.com/labC9S9.png)


| Column 1 | phase1 | phase2 |
| -------- | -------- | -------- |
| task1     | none    | none     |
| task2     | none    | none     |
| task3     | none    | none     |
| task4     | 0，1，2  | none    |
| task5     | 1，4     | 5       |
| task6     | 2，4     | none    |
| task7     | 1，3，6   | none     |
| task8     | 0，4，7   | none     |
| task9     | 5       | 4，6     |

![](https://i.imgur.com/ZtqVYSO.png)
![](https://i.imgur.com/Y8cxc04.png)

[時間跟CPU Idle次數](https://hackmd.io/jwQ09Ju7SbCcEOej21wnow?edit)
```c=
//fibd the position to insert
        NODE * current = readyQHead;
        int ready_need_to_be_checked = numOfReady;
        while(ready_need_to_be_checked>1){
            //printf("%s insert not one",newThread->name);
            NODE * next = current->next;
            if((*current).thread->priority < (*newThread).priority){
                break;
            }
            current = next;
            ready_need_to_be_checked--;
        }
        //insert
        newNode->next = current;
        newNode->last = current->last;
        current->last->next = newNode;
        current->last = newNode;
        if(current == readyQHead)
            readyQTail = newNode;
```

![](https://i.imgur.com/xo8Hl9g.png)
感覺錯
```c=
void FCFS(){
    bool CPUBusy = false;
    if(runNode != NULL && interrupt == true){ // if there will be interrupt, it is need to be execute
        CPUBusy = true;
        interrupt = false;
        task_run();
    }
    else if(readyQHead != NULL && runNode == NULL){
        runNode = readyQHead;
        deleteReadyQ(runNode);   //先離開ready queue
        CPUBusy = true;
        task_run();
    }
}
void RR(){
    bool CPUBusy = false;
    if(runNode != NULL&& interrupt == true){ // if there will be interrupt, it is need to be execute
        CPUBusy = true;
        interrupt = false;
        task_run();
    }
    while(readyQHead != NULL && runNode == NULL){
        runNode = readyQHead;
        deleteReadyQ(runNode);   //先離開ready queue
        CPUBusy = true;
        task_run();
    }
}
void PP(){
    bool CPUBusy = false;
    if(runNode != NULL){ // if there will be interrupt, it is need to be execute
        CPUBusy = true;
        task_run();
    }
    while(readyQHead != NULL || waitQHead != NULL){
        while(readyQHead==NULL){
            if(CPUBusy == true && runNode == NULL){
                printf("CPU idle.\n");
                CPUBusy = false;
            }
        }
        runNode = readyQHead;
        deleteReadyQ(runNode);   //先離開ready queue
        CPUBusy = true;
        task_run();
        if(interrupt){
            break;
        }
    }
}
```
![](https://i.imgur.com/EjzG6A2.png)
```

