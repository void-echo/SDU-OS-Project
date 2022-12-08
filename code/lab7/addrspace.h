// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "bitmap.h"
#include "copyright.h"
#include "filesys.h"
#include "noff.h"
#include "stats.h"
#include "translate.h"
#define UserStackSize 1024  // increase this as necessary!
// lab7--------------------------------
#define numPhysPages 5  // 固定分配的帧数

#ifndef SWAP_STRATEGY
#define SWAP_STRATEGY int
#define STR__FIFO__ 1
#define STR__CLOCK__ 2
#endif

class AddrSpace {
   public:
    AddrSpace(OpenFile *executable,
              char *filename);  // Create an address space,
                                // initializing it with the program
                                // stored in the file "executable"
    ~AddrSpace();               // De-allocate an address space

    void InitRegisters();  // Initialize user-level CPU registers,
                           // before jumping to user code

    void RestoreState();  // info on a context switch

    //-----------lab6-----------
    void Print();
    unsigned int getSpaceID() { return spaceID; }
    // lab7----------------------
    int FIFO(int badVAddr);
    int clock(int newPage);
    int Swap(int oldPage, int newPage);
    // void Translate(int addr,int* vpn, int *offset);
    int writeBack(int oldPage);
    bool notUsednotDirty() {
        return pageTable[virtualMem[p_vm]].use == 0 &&
               pageTable[virtualMem[p_vm]].dirty == 0;
    }
    bool notUsedbutDirty() {
        return pageTable[virtualMem[p_vm]].use == 0 &&
               pageTable[virtualMem[p_vm]].dirty == 1;
    }

   private:
    TranslationEntry *pageTable;  // Assume linear page table translation
                                  // for now!
    unsigned int numPages;        // Number of pages in the virtual
                                  // address space
    static BitMap *userMap;
    unsigned int spaceID;

    //-----------lab7-----------
    unsigned int StackPages;
    char *filename;
    NoffHeader noffH;
    Statistics *stats = new Statistics;
    int virtualMem[numPhysPages];  // FIFO页顺序存储
    int p_vm;                      // FIFO换出页指针
    int a;
};

#endif  // ADDRSPACE_H
