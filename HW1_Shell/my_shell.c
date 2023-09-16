#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include  <stdio.h>
#include  <sys/types.h>
int     myHelp();
int     myCd(char **args);
void     myEcho(char **args);
int     myRecord();
int     myReplay(char *args);
int     myPid(char *args,char *id);

int     myExit();
void    inserttail(char *newcommand);

char *commandMyself[] =       {"help","cd","echo","record","replay","mypid","exit"};
char *commandDetailMyself[] = { "show all build-in function info",
                                "change directory",
                                "echo the strings to standard output",
                                "show last-16 cmds you typed in",
                                "re-execute the cmd showed in record",
                                "find and print process-ids",
                                "exit shell"};

#define MAX_LINE 1024 // The maximum length command

int should_run  = 1;  // flag to determine when to exit program
int should_wait = 1;  // flag to determine if process should run in the background

pid_t pid;            // variable store the process ID for the use of fork in the backgorund

/****history(record)******************/
char RECODR[16];
struct node{
    char* str;
    struct node* next; 
};
typedef struct node NODE;
NODE * tail =  NULL;
NODE * head = NULL;
int numOfRecord = 0;
/****history(record)******************/

int myNumBuiltins(){
    return sizeof(commandMyself) /sizeof(char *);
}

char  line[1024];             
char  *argv[64];              

/****************************************************************
 * handle input and output redirection
 * **************************************************************/
void redirectIn(char *fileName){
    int in = open(fileName, O_RDONLY);
    dup2(in, 0);
    close(in);
}
void redirectOut(char *fileName){
    int out = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0600);
    dup2(out, 1);
    close(out);
}

/****************************************************************
 * Read: Read the command from standard input.
 * Parse: Separate the command string into a program and arguments.
 * Execute: Run the parsed command.
 * **************************************************************/
char * tokenize(char *input){
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((MAX_LINE * 2) * sizeof(char));

    // add spaces around special characters
    for (i = 0; i < strlen(input); i++) {
        if (input[i] != '>' && input[i] != '<' && input[i] != '|') {
            tokenized[j++] = input[i];
        } 
        else {
            tokenized[j++] = ' ';
            tokenized[j++] = input[i];
            tokenized[j++] = ' ';
        }
    }
    tokenized[j++] = '\0';

    // add null to the end
    char *end;
    end = tokenized + strlen(tokenized) - 1;
    end--;
    *(end + 1) = '\0';

    return tokenized;
}

void run(char *args[]){
    int count=0;
    //if it's not exit command
    if (strcmp(args[0], "exit") != 0){
        //fork() a child process to run the command
        
        //<Execute>
        int find = 0;
        for(int i = 0 ;i < myNumBuiltins();i++){
            if (strcmp(commandMyself[i], args[0]) == 0) {
                switch (i){
                    case 0: myHelp();find = 1; break;
                    case 1: myCd(args); find = 1; break;
                    case 2: myEcho(args); find = 1; break;
                    case 3: myRecord(); find = 1; break;
                    case 4: myReplay(args[1]); find = 1; break;
                    case 5: myPid(args[1],args[2]); find = 1; break;
                }
            }
        }
        if(find){
            redirectIn("/dev/tty");          
            redirectOut("/dev/tty");
            return;
        }
        pid = fork();
        if (pid < 0) { 
            fprintf(stderr, "Fork Failed");
        }
        else if ( pid == 0) { // child process 
            //run execv
            execvp(args[0], args);
        }
        else {                 // parent process
            if (should_wait) { // if should_wait then it's not run in background 
                waitpid(pid, NULL, 0);
            } 
            else {             // run in background
                should_wait = 0;
            } 
        }
        // dev/tty is a special file, representing the terminal for the current process.
        // dev/tty doesn't 'contain' anything as such, but I can read from it and write to it.
        redirectIn("/dev/tty");           //redirect read from the terminal for the current processs
        redirectOut("/dev/tty");          //redirect write from the terminal for the current processs
    }
    else {                    //if it's exit command
        myExit();
        exit(0);
        should_run = 0;
    }
}

void createPipe(char *args[]){
    int fd[2];
    pipe(fd);

    dup2(fd[1], 1);
    close(fd[1]);

    run(args);

    dup2(fd[0], 0);
    close(fd[0]);
}

int main(void){
    char *args[MAX_LINE]; // command line arguments
    /****************************************************************
     * customize your hello-message when your shell starts running.
     * The messages will not affect your score.
     * **************************************************************/
    printf("=====================================================\n");
    printf("* Welcome to my little shell:                       *\n");
    printf("*                                                   *\n");
    printf("* Type \"help\" to see builtin functions              *\n");
    printf("*                                                   *\n");
    printf("* If you want to do things below:                   *\n");
    printf("* + redirection: \">\" or \"<\":                        *\n");
    printf("* + pipe: \"|\"                                       *\n");
    printf("* + background: \"&\"                                 *\n");
    printf("* Make sure they are separated by \"(space)\".        *\n");
    printf("*                                                   *\n");
    printf("* Have fun!!                                        *\n");
    printf("=====================================================\n");

    /****************************************************************
     * Read: Read the command from standard input.
     * Parse: Separate the command string into a program and arguments.
     * Execute: Run the parsed command.
     * **************************************************************/

    while (should_run) { 
        printf(">>> $ ");
        fflush(stdout);
        char input[MAX_LINE];
        // <<Read>>
        fgets(input, MAX_LINE, stdin);            
    
     
        //If user input a line with only “space“ or “\t” characters,do nothing, and print another prompt symbol.
        if(strlen(input) == 0 && (input[0] == '\t'))continue;
        if(input[0] == ' ') continue;
     
        //record the command 
        char *tmp = malloc(sizeof(char) * 128);   
        for(int j = 0;j < strlen(input) + 1;j++){
            tmp[j] = input[j];
        } 
        
        inserttail(tmp);
        
        char *tokens;
        tokens = tokenize(input);

        if (tokens[strlen(tokens) - 1] == '&') {
            should_wait = 0;
            tokens[strlen(tokens) - 1] = '\0';
        }

        char *arg = strtok(tokens, " ");
        int i = 0;
        while (arg) {
            //handle input and output redirection
            if (*arg == '<') {
                redirectIn(strtok(NULL, " "));
            } 
            else if (*arg == '>') {
                redirectOut(strtok(NULL, " "));
            } 
            else if (*arg == '|') {
                args[i] = NULL;
                createPipe(args);
                i = 0;
            } 
            else {
                args[i] = arg;
                i++;
            }
            arg = strtok(NULL, " ");
        }
        args[i] = NULL;

        // <<Execute>>
        run(args);
        if(numOfRecord !=0 )
        // <<Parse>>
        // If command run in backgorund print 
        if(!should_wait){ // if fun in background print 
            fprintf(stderr,"[Pid]: %d\n",pid);
            should_wait = 1;
        }
    }
    return 0;
}

/*****************help***************************************************************/
int myHelp(){
    int i;
    printf("-------------------------------------------------------------\n");
    printf("WeiHsinYeh's Shell          \n");
    printf("Welcome to my little shell\n");
    printf("The following are built in:\n");
    for (i = 0; i < 7; i++) {
        printf("%d: %s\t\t%s\n",i+1, commandMyself[i],commandDetailMyself[i]);
    }
    printf("\nUse the \"man\" command for information on other programs.\n");
    printf("-------------------------------------------------------------\n");
}
/*****************help***************************************************************/

/*****************cd*****************************************************************/
int myCd(char **args){
    if(args[1] == NULL){
        printf("expected argument to \"cd\"\n"); 
    }else{
        if(chdir(args[1]) != 0){  
            printf("error");
        }
    }
    return 1;
}
/*****************cd*****************************************************************/

/*****************echo***************************************************************/
void myEcho(char **args){
	args++;
	if(strcmp(*args,"-n")==0){
		args++;
		while(*args!=NULL){		
            printf("%s ",*args);		
			args++;
		}
	}
    else{		
		while(*args!=NULL){		
            printf("%s ",*args);	
			args++;
		}	
        printf("\n");	
	}
	return;
}
/*****************echo***************************************************************/

/*****************replay*************************************************************/
int myReplay(char *args){
    if (args == NULL) {
        printf("expected argument\n");
        return 1;
    }
    else {
        char str[10];
        strcpy(str,args);
        char *ptr;
        int index = strtoumax(str,&ptr,10); // the number of the command want to replay
        
        // If the number of commmand want to be executed isn's exist
        if(!(index >= 1 && index <=numOfRecord)){
            fprintf(stderr, "wrong args\n");
            return 1;
        }

        NODE* current = head;
        int status;
        char *argss[MAX_LINE]; // command line arguments

        // Find the command in the record
        for(int i=1;i<= numOfRecord;i++){
            if(index == i){
                //<Read>
                char* input = NULL;
                char* buffer = NULL;
                input = (char*)malloc(sizeof(char) * 128);
                buffer = (char*)malloc(sizeof(char) * 128);
                for(int i=0;i<strlen((*current).str);i++){
                    input[i] = (*current).str[i];
                    buffer[i] = (*current).str[i];
                }

                //Record the command
                inserttail(buffer);
                
                //<Parse>
                char *tokens;
                tokens = tokenize(input);
                should_wait = 1;
                if (tokens[strlen(tokens) - 1] == '&') {
                    should_wait = 0;
                    tokens[strlen(tokens) - 1] = '\0';
                }
                char *argk = strtok(tokens, " ");
                int i = 0;
                while (argk) {
                    if (*argk == '<') {
                        redirectIn(strtok(NULL, " "));
                    } 
                    else if (*argk == '>') {
                        redirectOut(strtok(NULL, " "));
                    } 
                    else if (*argk == '|') {
                        argss[i] = NULL;
                        createPipe(argss);
                        i = 0;
                    } 
                    else {
                        argss[i] = argk;
                        i++;
                    }
                    argk = strtok(NULL, " ");
                }
                argss[i] = NULL;

                run (argss);
                
            }
            else{
                current = (*current).next;
            }
        }
    }
    return 1;
}
/*****************replay*************************************************************/

/*****************mypid**************************************************************/
int myPid(char *args,char *id){

    if(args[1] == 'i'){      // print the process ID of the process it self
        int pid_self = getpid();
        printf("%d\n",pid_self);
    }
    else if(args[1] == 'p'){ // print the parent process ID of the process[ID]
        
        // handle the parameter ID and concate to the destination
        // destination /proc/ID/status
        char destination[128] = "/proc/";
        strcat(destination, id);
        strcat(destination,"/status");
        
        // open the file 
        FILE *fp = fopen(destination,"r");
        if(fp == NULL){
            printf("Process doesn's exist\n");
            return 1;
        }
        char str[10];
        // Name:   systemd
        // Umask:  0000
        // State:  S (sleeping)
        // Tgid:   1
        // Ngid:   0
        // Pid:    1
        // PPid:   0
        // parent is in the 7th parameter
        for(int i = 0;i < 7;i++){ 
            fgets(str,128,fp);
        }
        const char* d = "PPid:\t";
        char *p;
        p = strtok(str,d);
        while(p != NULL){
            fprintf(stderr, "%s",p);
            p = strtok(NULL,d);
        }
        fclose(fp);
    }
    else if(args[1] == 'c'){ // print the child process ID of the process[ID]

        // make the string : /proc/ID/task/ID/children
        char child[128] = "/proc/";      
        strcat(child, id); 
        strcat(child, "/task/");
        strcat(child, id);
        strcat(child, "/children");  // see its' children
        FILE *fp = fopen(child, "r" );
        if (fp == NULL) {
            printf("Process doesn't exists\n");
            return 1;
        }
        char str[128] = "Process doesn't have child processs";
        fgets(str, 128, fp); 
        printf("%s\n", str);
        fclose(fp);
    }
    return 1;
}
/*****************mypid**************************************************************/

/*****************record*************************************************************
 * Your shell will always record the last-16 commands that user used
 * in the shell. When user type the “record” command, the shell will
 * print the last-16 commands to stdout, including “record” itself.
 * The biggest number indicate the latest command being used
*************************************************************************************/
void inserttail(char *newcommand){
    // If the command is replay then not record it
    if(((newcommand[0]=='r'&&newcommand[1]=='e')&&
        (newcommand[2]=='p'&&newcommand[3]=='l'))&&
        (newcommand[4]=='a'&&newcommand[5]=='y')){
            return;
        }
    // Create record by linked list
    // Create a Node to store the command
    NODE * newNode = (NODE*) malloc (sizeof(NODE));
    char * buffer = (char*)malloc(sizeof(char) * 128);
    for(int i=0;i<strlen(newcommand);i++){
        buffer[i] = newcommand[i];
    }
    (*newNode).next = NULL;
    (*newNode).str = buffer;

    if(numOfRecord == 0){        //If this is the first command
        head = newNode;
        numOfRecord++;
    }
    else if(numOfRecord != 16){  // If the # of record command don't achieve 16
        (*tail).next = newNode;
        numOfRecord++;
    }
    else{                        // If the # of record command achieve 16 just adjust head and push new node
        head = (*head).next;
        (*tail).next = newNode;
    }
    tail = newNode;
    return;
}
int myRecord(){
    NODE* current = head;
    // print the record history
    printf("history cmd:\n");
    int i=1;
    while(current != NULL){
        printf("%2d: %s",i++ ,(*current).str);
        current = (*current).next;
    }
}
/*****************record*************************************************************/

/*****************exit***************************************************************/
int myExit(){
    printf("See you next time\n");
    exit(0);
}
/*****************exit***************************************************************/