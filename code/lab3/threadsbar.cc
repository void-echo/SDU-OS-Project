#define N_THREADS  10    // the number of threads
#define N_TICKS    10000  // the number of ticks to advance simulated time
#define MAX_NAME  16 // the maximum lengh of a name

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "copyright.h"
#include "system.h"
#include "synch.h"



Thread *threads[N_THREADS];
char threads_names[N_THREADS][MAX_NAME];  
Semaphore *barrier,*mutex,*barrier1,*mutex1;  
Thread *current_thread;

void MakeTicks(int n) {
    
    for(int i=0;i<n;i++){ }
    //sleep(1);

} // advance n ticks of simulated time


int count = 0;
int count1 = 0;


void BarThread(_int which)
{
    //printf("Thread %d sleep\n", which);
    //MakeTicks(N_TICKS);

   
    mutex1->P();
        count1 = count1+1;
        if(count1 == 10){
            barrier1->V();
	    //printf("begin rendezvous\n");
        }
    mutex1->V();
    barrier1->P();
    barrier1->V();


    printf("Thread %d rendezvous\n", which);
    mutex->P();
        count = count+1;
        if(count == 10){
            barrier->V();
	    printf("Thread %d is the last\n", which);
        }
    mutex->V();

    
    barrier->P();
    //printf("barrier->P() barrier value: %d \n", barrier->value);
    barrier->V();
    //printf("barrier->V() barrier value: %d \n", barrier->value);
    printf("Thread %d critical point\n", which);

}


void ThreadsBarrier()
{
    mutex = new Semaphore("mutex", 1);
    mutex1 = new Semaphore("mutex1", 1);
    barrier = new Semaphore("barrier", 1);
    barrier1 = new Semaphore("barrier1", 1);
    barrier->P();
    barrier1->P();
    // create and fork N_THREADS of consumer threads 
    for (int i=0; i < N_THREADS; i++) {
        // this statemet is to form a string to be used as the name for thread i. 
        sprintf(threads_names[i], "%d", i);
        threads[i] = new Thread(threads_names[i]);
	threads[i]->Fork(BarThread, i);
    };
}
