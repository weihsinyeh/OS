#include<linux/module.h>
#include<linux/sched.h>
#include<linux/fs.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include<linux/stat.h>
#include<linux/version.h>
#include<linux/pid.h>
#include<linux/pid_namespace.h>
#include<linux/thread_info.h>
#define PROC_NODE_ROOT_NAME "thread_info"
#define __ASM_GENERIC_CURRENT_H

static char *str = NULL;

static struct proc_dir_entry *proc_test_root;
static struct proc_dir_entry *proc_file1, *proc_file2;
struct Mytask_struct{
    struct Mytask_struct * next;
    u64 utime;
    int threadId;
    unsigned long nvcsw;
    unsigned long nivcsw;
};
typedef struct Mytask_struct NODE;
NODE * firstnode = NULL;
NODE * lastnode = NULL;
NODE * currentnode = NULL;
int processID;
int counter= 0;
char content[4096];
int length = 0;
int threadIDDDD;
/****************************************************************************
 * When the proc file is read, the thread relationships and the above timing
 * information of all the recorded threads should be output to the reader.
 * **************************************************************************/
int my_proc_show(struct seq_file *m,void *v){
    currentnode = firstnode;
    seq_printf(m,"PID:%d",processID);
    length += sprintf(content+length,"\0");
    seq_printf(m,"%s",content);
    return 0;
}
static int my_proc_open(struct inode *inode,struct file *file){
    return single_open(file,my_proc_show,NULL);
}
char s[4096] = "HelloWorld!\n"; 
int len = sizeof(s);
void my_store(int threadID){
    NODE * newStruct = kzalloc(sizeof(struct Mytask_struct),GFP_KERNEL);
    /*threadID****************************************/
    (*newStruct).threadId = threadID;
    (*newStruct).next = NULL;
    /*threadID****************************************/
    struct task_struct *task = pid_task(find_get_pid(threadID),PIDTYPE_PID);
    if(task!=NULL){
        /*************************************************
         * *Note: thread execution time can be obtained from utime, and context switch count
         * = nvcsw + nivcsw, where utime, nvcsw and nivcsw are fields of a task structure.
         * ***********************************************/
        /*time************************************/
        (*newStruct).utime = task->utime;
        (*newStruct).nvcsw = task->nvcsw;
        (*newStruct).nvcsw = task->nivcsw;
        /*time************************************/
        if(firstnode == NULL) firstnode = newStruct;
        else (*lastnode).next = newStruct;
        lastnode = newStruct;
        currentnode = newStruct;
        threadIDDDD = (*currentnode).threadId;
        printk(KERN_EMERG "currentthreadID%d.\n",(*currentnode).threadId);
        printk(KERN_EMERG "currentnvcsw:%ld",(*currentnode).nvcsw);
        unsigned long contextswitch = (*currentnode).nvcsw + (*currentnode).nivcsw;

        length += sprintf(content+length,"\n\tThreadID:%d Time:%llu(ms) context switch times:%ld",(*newStruct).threadId,((*newStruct).utime)/1000000,contextswitch);
    }
    else{
        printk(KERN_EMERG "error NULL:%d",(*currentnode).threadId);
    }
}

static ssize_t my_proc_write(struct file *file,const char __user *buffer,size_t count,loff_t *f_pos){
    char *tmp = kzalloc((count + 1),GFP_KERNEL);
    if(!tmp)
        return -ENOMEM;
    if(copy_from_user(tmp,buffer,count)){
        kfree(tmp);
        return -EFAULT;
    }
    kfree(str);
    int threadID;
    if(counter == 0){
        kstrtoint(tmp,10,&processID);
        counter++;
    }
    else{
        /*****************************************************************************************
         * a) User threads will write their thread ids to the proc file. When a user thread writes
         * its id, the kernel module should record the thread id, get the thread execution time
         * and context switch count of the thread.
         * ***************************************************************************************/
        kstrtoint(tmp,10,&threadID);
        my_store(threadID);
        printk(KERN_EMERG "counter : %d !!!\n",counter);
    }
    str = tmp;

    return count;
}
//3. You have to implement file operations of the proc file
static struct proc_ops my_fops = {
    .proc_open = my_proc_open,
    .proc_release = single_release,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_write = my_proc_write,    
};

static int __init create_proc_file(void){
    struct proc_dir_entry* file = NULL;
    //1. You have to write a kernel module named My_proc
    //2. 2. The kernel module has to create a proc file with pathname
    //   /proc/thread_info during its initialization/loading
    file = proc_create(PROC_NODE_ROOT_NAME,0666,NULL,&my_fops);
    if(!file){
        printk(KERN_EMERG "create_proc_file error !!!\n");
        return -ENOMEM;
    }
    else{
        printk(KERN_EMERG "create_proc_file finish.\n");
    }
    return 0;
}
static void __exit remove_proc_file(void){
    remove_proc_entry(PROC_NODE_ROOT_NAME,NULL);
    if(str){
        kfree(str);
    }
    printk(KERN_EMERG "remove_proc_file\n");
}
MODULE_LICENSE("GPL");
module_init(create_proc_file);
module_exit(remove_proc_file);
