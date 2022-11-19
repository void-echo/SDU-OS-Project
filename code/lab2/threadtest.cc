// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread(_int which) {
    int num;
    int times_run = 5;

    for (num = 0; num < times_run; num++) {
        printf("*** thread %d looped %d times\n", (int)which, num);
        if (num != times_run - 1) {
            // if not the last time, yield the CPU to another thread
            currentThread->Yield();
        }
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest() {
    printf("ThreadTest in LAB2, Starting!\n");
    DEBUG('t', "Priority Test in LAB2, Starting!\n");

    for (int i = 0; i < 4; i++) {
        char* name = new char[10];
        sprintf(name, "Thread %d", 4-i);
        Thread *t = new Thread(name, 4-i);
        t->Fork(SimpleThread, 4-i);
    }
    // SimpleThread(0);
}
