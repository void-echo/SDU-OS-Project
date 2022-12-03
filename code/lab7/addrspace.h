// addrspace.h
//  Data structures to keep track of executing user programs
//  (address spaces).
//
//  For now, we don't keep any information about address spaces.
//  The user level CPU state is saved and restored in the thread
//  executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "../bin/noff.h"
#include "bitmap.h"
#include "copyright.h"
#include "filesys.h"
#include "translate.h"

#define MemPages 6

#define UserStackSize 1024  // increase this as necessary!
#define StackPages (divRoundUp(UserStackSize, PageSize))

class AddrSpace {
   public:
    AddrSpace(OpenFile *executable);  // Create an address space,
                                      // initializing it with the program
                                      // stored in the file "executable"
    ~AddrSpace();                     // De-allocate an address space
    void InitRegisters();             // Initialize user-level CPU registers,
                                      // before jumping to user code

    void SaveState();     // Save/restore address space-specific
    void RestoreState();  // info on a context switch

    void Print();
    int getSpaceID() { return spaceID; }

    void InitPageTable();   //初始化AddrSpace的PageTable的基本信息
    void InitInFileAddr();  //初始化inFileAddr和type
    void clock(int newPage);  //调用translate和swap实现二次机会增强的置换算法
    void Translate(
        int addr, unsigned int *vpn,
        unsigned int *offset);  //将addr对应的虚拟页页号和页内偏移量offset算出来
    void Swap(
        int oldPage, int newPage,
        int badVAddr);  //调用WriteBack和ReadIn实现mainMemory中的oldPage替换成newPage
    void WriteBack(int oldPage, int badVAddr);  //将oldPage这一个页写回
    void ReadIn(int newPage, int badVAddr);     //将newPage写入到mainMemory

   private:
    TranslationEntry *pageTable;  // Assume linear page table translation
                                  // for now!
    unsigned int numPages;        // Number of pages in the virtual
                                  // address space

    int spaceID;

    static BitMap *bitmap;

    OpenFile *executable;  //当下正在执行的用户文件的指针OpenFile *executable

    NoffHeader noffH;          //当下正在执行的用户文件的NoffHeader
    int virtualMem[MemPages];  //存储当前占用内存空间的虚拟页
    int p_vm;                  //指针
};

#endif  // ADDRSPACE_H
