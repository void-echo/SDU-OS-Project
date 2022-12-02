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


#define N_THREADS  10    // the number of threads
#define N_TICKS    10000  // the number of ticks to advance simulated time

Thread *threads[N_THREADS];
Semaphore *mutex;
Semaphore *barrier;
Semaphore *createbar;
int count;

void MakeTicks(_int which){
    int x = rand();
    int y = x % 10;
    if(y>5){
        threads[which]->Yield();
    }
}
void BarThread(_int which){
    MakeTicks(which);
    printf("Thread %d rendezvous\n", which);

    mutex->P();
    count++;
    if(count == N_THREADS){
        printf("Thread %d is the last\n", which);
        barrier->V();
    }
    mutex->V();
    barrier->P();
    barrier->V();

    printf("Thread %d critical point\n", which);
}
void ThreadsBarrier(){
     DEBUG('t', "Entering BarrierTest");
     srand(time(0));
     count = 0;
     mutex = new Semaphore("mutex",1);
     barrier = new Semaphore("barrier",0);
     createbar = new Semaphore("createbar",0);
    
     for(int j = 0;j<N_THREADS;j++){
        threads[j] = new Thread("thread"+j);
     }

     for(int i = 0; i < N_THREADS; i++) {
        threads[i]->Fork(BarThread, i);
     };
     
}
