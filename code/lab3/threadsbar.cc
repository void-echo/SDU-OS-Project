#define N_THREADS  10    // the number of threads
#define N_TICKS    10000  // the number of ticks to advance simulated time
#define MAX_NAME  16 // the maximum lengh of a name

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "copyright.h"
#include "system.h"
#include "synch.h"



Thread *threads[N_THREADS];
char threads_names[N_THREADS][MAX_NAME];  
Semaphore *barrier,*mutex;  
Thread *current_thread;

void MakeTicks(_int which){
    int x = rand();
    if(x%2==0){
        threads[which]->Yield();
    }

}


int count = 0;
int count1 = 0;


void BarThread(_int which)
{
    MakeTicks(which);

    printf("Thread %d rendezvous\n", which);

    
    mutex->P();
        count = count+1;
        if(count == 10){
	    printf("Thread %d is the last\n", which);
            barrier->V();

        }
    mutex->V();

    
    barrier->P();
    barrier->V();
    printf("Thread %d critical point\n", which);

}


void ThreadsBarrier()
{
srand(time(0));
    mutex = new Semaphore("mutex", 1);
    barrier = new Semaphore("barrier", 1);

    barrier->P();
    // create and fork N_THREADS of consumer threads 
    for (int i=0; i < N_THREADS; i++) {
        
        threads[i] = new Thread("thread"+i);
    };
    for (int i=0; i < N_THREADS; i++) {
	threads[i]->Fork(BarThread, i);
    };
}
