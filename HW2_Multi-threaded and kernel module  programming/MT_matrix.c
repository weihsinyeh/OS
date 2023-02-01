#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <linux/cn_proc.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <utime.h>
#include<sys/types.h>
#include<sys/stat.h>
typedef struct{
   int * matrix1;
   int * matrix2;
   int * matrix3;
   int * info;
   int  id;
   int  numberOfThreads;
}ThreadedArgs;

pthread_mutex_t count_mutex;
// 子執行緒函數
void* child(void *args) {
   ThreadedArgs *pass = (ThreadedArgs *) args; // pass parameter
   int *matrix1 = (*pass).matrix1;
   int *matrix2 = (*pass).matrix2;
   int *result = (*pass).matrix3;

   int row1 = (*((*pass).info));
   int col1 = (*(((*pass).info)+1));
   int row2 = (*(((*pass).info)+2));
   int col2 = (*(((*pass).info)+3));
   int numberOfThreads = (*pass).numberOfThreads;
   int rank = (*pass).id;

   /*******************************************************************************
    * 3. Each worker thread should perform a part of the matrix multiplication job.
   ********************************************************************************/
   // calculate the displacement and count of the loading of every thread
   int total = row1 * col2; //100
   int *counts = malloc(sizeof(int) * numberOfThreads);
   int *offsets = malloc(sizeof(int) * numberOfThreads); //由offset算出是從哪個二維陣列元素開始的
   int remainder = total % numberOfThreads; 
   int sum = 0;
   for(int id = 0;id < numberOfThreads;id++){
      counts[id] = total / numberOfThreads ; 
      if(id == 0){
         counts[id] += remainder;
      }
      offsets[id] = sum; 
      sum += counts[id];
   }   
   //counts[id] 代表此thread 要計算多少元素
   //offsets[id] 代表此thread 要計算的元素應該從哪個開始
   //result[i/col2][i%col2]

   int upper = (rank == numberOfThreads-1) ? total : offsets[rank+1];
   for(int i=offsets[rank];i < upper;i++){
      result[i] = 0;
      int row = i/col2;
      int col = i%col2;
      for(int k = 0;k <col1;k++){
         result[i] += matrix1[row* col1 + k] * matrix2[k*col2 + col];
      }
   }

   pthread_mutex_lock(&count_mutex);           //avoid critical section
   /*******************************************
    * 4. Each worker thread should write its thread ID
    * to the proc file right before its termination.
    * *****************************************/
   int threadID = gettid(); 
   int processID = getpid();
   char threadIDString[128];
   char processIDString[128];
   sprintf(threadIDString,"%d",threadID);  //整數轉字串
   sprintf(processIDString,"%d",processID);//整數轉字串
   char pass1[128] = "";
   strcat(pass1,processIDString);
   strcat(pass1," ");
   strcat(pass1,threadIDString);
   
   /*
   4. Each worker thread should write its thread ID to the proc file right before its termination.
   (May cause race condition, slide 7 and 8 shows description of race condition)
   */
   char buf[100];
	int fd = open("/proc/thread_info", O_RDWR);
	lseek(fd, 0 , SEEK_SET);
	write(fd, threadIDString, strlen(threadIDString));
	lseek(fd, 0 , SEEK_SET);
   close(fd);   
   
   /*******************************************/
   pthread_mutex_unlock(&count_mutex);         //avoid critical section
   pthread_exit(NULL);
}

// 主程式
/*************************************************************************************************
 * 8. The executable and parameters of your multithread program:
 *./MT_matrix [number of worker threads] [file name of input matrix1] [file name of input matrix2]
 *************************************************************************************************/
int main(int argc,char * argv[]) {

   int row1,col1,row2,col2;
   //read parameter
   int numberOfThreads = atoi(argv[1]);
   char *filenameOfInputMatrix1 = argv[2];
   char *filenameOfInputMatrix2 = argv[3];
   /*******************************************
    * Read Matrix1
    * *****************************************/
   FILE* readMatrix1;
   readMatrix1 = fopen (filenameOfInputMatrix1,"r");
   if(readMatrix1 == NULL)  printf("file can't be opened\n");
   fscanf(readMatrix1,"%d ",&row1);
   fscanf(readMatrix1,"%d\n",&col1);
   int *matrix1 = malloc(sizeof(int) * row1 * col1); // 配置記憶體
   for(int i = 0; i < row1;i++){
      for(int j = 0;j < col1;j++){
         fscanf(readMatrix1,"%d",&matrix1[i * col1 + j]);
      }
   }
   fclose(readMatrix1);
   /*******************************************
    * Read Matrix2
    * *****************************************/
   FILE* readMatrix2;
   readMatrix2 = fopen (filenameOfInputMatrix2,"r");
   if(readMatrix2 == NULL)  printf("file can't be opened\n");
   fscanf(readMatrix2,"%d",&row2);
   fscanf(readMatrix2,"%d",&col2);
   int *matrix2 = malloc(sizeof(int) * row2 * col2); // 配置記憶體
   for(int i = 0; i < row2;i++){
      for(int j = 0;j < col2;j++){
         fscanf(readMatrix2,"%d",&matrix2[i * col2 + j]);
      }
   }
   fclose(readMatrix2);
   /******************************************/
   int processID = getpid();
   char buf[4096];
   char processIDString[128];
	int fd = open("/proc/thread_info", O_RDWR);
   sprintf(processIDString,"%d",processID);         //整數轉字串
	lseek(fd, 0 , SEEK_SET);
	write(fd, processIDString, strlen(processIDString));
	lseek(fd, 0 , SEEK_SET);
   close(fd); 

   pthread_t *t = malloc(sizeof(pthread_t) * numberOfThreads) ;
   int *result = malloc(sizeof(int) * row1 * col2); // 配置記憶體
   int info[4] = {row1,col1,row2,col2};             // 輸入的資料
   struct timespec start,finish;
   double elapsed;
   clock_gettime(CLOCK_MONOTONIC,&start);

   /************************************************************
    * 2 . The program starts with a single main/parent thread, 
    * which is responsible for creating multiple worker threads.
    * **********************************************************/
   for(int i = 0;i < numberOfThreads;i++){          // 建立子執行緒，傳入 input 進行計算
      ThreadedArgs* pass = malloc(sizeof(ThreadedArgs));
      (*pass).info = info;
      (*pass).matrix1 = matrix1;
      (*pass).matrix2 = matrix2;
      (*pass).matrix3 = result;
      (*pass).id = i; 
      (*pass).numberOfThreads = numberOfThreads;
      pthread_create(&t[i], NULL, child, (void*) pass);
   } 
   for(int i = 0;i < numberOfThreads;i++){
      pthread_join(t[i],NULL);
   }
   clock_gettime(CLOCK_MONOTONIC,&finish);
   elapsed = (finish.tv_sec - start.tv_sec);
   elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

   /***************************************************************************
    * 5. After completing the matrix multiplication, the main thread has to read 
    * the proc file and print the following resulting information on the console
    * 1. Main/parent thread ID
    * 2. Each worker/child thread ID and execution time and context switch times.
   ****************************************************************************/
   fd = open("/proc/thread_info", O_RDWR);
	read(fd,buf,4096);
   //strcat(buf,"\0");
   printf("%s",buf);
   printf("\n");
   close(fd); 
   /***************************************************************************
    * 6. After completing the matrix multiplication, the program also has to save 
    * the result of the matrix multiplication (i.e. the result matrix) to a file 
    * named as result.txt.
    ***************************************************************************/
   FILE *writeMatrix = fopen("result.txt","w");
   fprintf(writeMatrix,"%d %d\n",row1,col2);
   for(int i=0;i<row1;i++){
      for(int j=0;j<col2;j++)
         fprintf(writeMatrix,"%d ",result[i*col2 + j]);
      fprintf(writeMatrix,"\n");
   }
   fclose(writeMatrix);
   free(result); // 釋放記憶體

	printf("it took %f seconds to execute \n",elapsed);
	
   
   return 0;
}