// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"


#include "bitmap.h"
#include "copyright.h"
#include "system.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
#define MemPages 6
//////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

BitMap *Mmbmp = new BitMap(NumPhysPages);  //用于分配mainMemory中物理页的位图。
BitMap *SwapBitmap = new BitMap(NumPhysPages);  // SWAP的位图。

OpenFile *SwapFile = fileSystem->Open("SWAP");

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *exe) {
    if (!SwapFile) {
        printf("not found!\n\n\n\n\n");
    }

    unsigned int size;
    executable = exe;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    numPages = divRoundUp(noffH.code.size, PageSize) +
               divRoundUp(noffH.initData.size, PageSize) +
               divRoundUp(noffH.uninitData.size, PageSize) + StackPages;
    size = (MemPages + StackPages) * PageSize;
    printf("numPages is %d\n", numPages);
    printf("numPages = %d + %d + %d + %d\n",
           divRoundUp(noffH.code.size, PageSize),
           divRoundUp(noffH.initData.size, PageSize),
           divRoundUp(noffH.uninitData.size, PageSize), StackPages);
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
          size);
    // zero out the entire address space, to zero the unitialized data segment
    // and the stack segment
    //将整个地址空间清零，将单元化的数据段和堆栈段清零。
    bzero(machine->mainMemory, size);

    InitPageTable();
    // 将代码和数据段复制到内存中
    InitInFileAddr();
    printf("当前内容为：\n");
    Print();
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() { delete[] pageTable; }

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
    int i;

    for (i = 0; i < NumTotalRegs; i++) machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::Print() {  // print函数要修改
    printf("page table dump: %d pages in total\n", numPages);
    printf("============================================\n");
    printf("\tVirtPage, \tPhysPage, \tuse,\tdirty\n");
    for (int i = 0; i < numPages; i++) {
        printf("\t%d, \t\t%d,\t\t%d,\t%d\n", pageTable[i].virtualPage,
               pageTable[i].physicalPage, pageTable[i].use, pageTable[i].dirty);
    }
    printf("============================================\n\n");
}

void AddrSpace::InitPageTable() {
    // int StackPages =divRoundUp(UserStackSize,PageSize);
    p_vm = 0;
    pageTable = new TranslationEntry[numPages];
    for (int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].use = 0;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
        pageTable[i].inFileAddr = -1;
        //初始化inFileAddr为-1，具体值会在InitFileAddr中计算出。
        if (i >= numPages - StackPages) pageTable[i].type = vmuserStack;
        //最后，将前MemPages个虚页的内容分配mainMemory的物理页，准备将其写入到mainMemory中去，写入过程由InitFileAddr完成。
        if (i < MemPages) {
            virtualMem[p_vm] = pageTable[i].virtualPage;  //==i
            p_vm = (p_vm + 1) % MemPages;
            pageTable[i].physicalPage = Mmbmp->Find();

            pageTable[i].valid = TRUE;
            pageTable[i].use = 1;  //刚进来use为1
        } else {
            pageTable[i].physicalPage = -1;
            pageTable[i].valid = FALSE;
        }
    }
}

void AddrSpace::InitInFileAddr() {
    if (noffH.code.size > 0) {
        unsigned int numP = divRoundUp(noffH.code.size, PageSize);

        for (int i = 0; i < numP; i++) {
            pageTable[i].inFileAddr = noffH.code.inFileAddr + i * PageSize;
            pageTable[i].type = vmcode;
            if (pageTable[i].valid) {
                executable->ReadAt(
                    &(machine
                          ->mainMemory[pageTable[i].physicalPage * PageSize]),
                    PageSize, pageTable[i].inFileAddr);
            }
        }
    }
    if (noffH.initData.size > 0) {
        unsigned int numP, firstP;
        numP = divRoundUp(noffH.initData.size, PageSize);
        firstP = divRoundUp(noffH.initData.virtualAddr, PageSize);
        for (int i = firstP; i < numP + firstP; i++) {
            pageTable[i].inFileAddr =
                noffH.initData.inFileAddr + (i - firstP) * PageSize;
            pageTable[i].type = vminitData;
            if (pageTable[i].valid) {
                executable->ReadAt(
                    &(machine
                          ->mainMemory[pageTable[i].physicalPage * PageSize]),
                    PageSize, pageTable[i].inFileAddr);
            }
        }
    }
    if (noffH.uninitData.size > 0) {
        unsigned int numP, firstP;
        numP = divRoundUp(noffH.uninitData.size, PageSize);
        firstP = divRoundUp(noffH.uninitData.virtualAddr, PageSize);
        for (int i = firstP; i < numP + firstP; i++) {
            pageTable[i].type = vmuninitData;
            if (pageTable[i].valid) { /*brzero();*/
            }
        }
    }
}

void AddrSpace::Translate(int addr, unsigned int *vpn, unsigned int *offset) {
    // int StackPages =divRoundUp(UserStackSize,PageSize);
    int page = -1;
    int off = 0;
    if (addr >= numPages * PageSize - UserStackSize)
    // addr位于pageTable的userStack段中
    {
        int userPages = numPages - StackPages;
        page = userPages + (addr - userPages * PageSize) / PageSize;
        off = (addr - userPages * PageSize) % PageSize;
    } else if (noffH.uninitData.size > 0 &&
               addr >= noffH.uninitData.virtualAddr)
    // addr位于uninitData
    {
        page = divRoundUp(noffH.code.size, PageSize) +
               divRoundUp(noffH.initData.size, PageSize) +
               (addr - noffH.uninitData.virtualAddr) / PageSize;
        off = (addr - noffH.uninitData.virtualAddr) % PageSize;
    } else if (noffH.initData.size > 0 && addr >= noffH.initData.virtualAddr) {
        // addr位于initData
        page = divRoundUp(noffH.code.size, PageSize) +
               (addr - noffH.initData.virtualAddr) / PageSize;
        off = (addr - noffH.initData.virtualAddr) % PageSize;
    } else {
        // addr位于code中
        page = addr / PageSize;
        off = addr % PageSize;
    }
    *vpn = page;
    *offset = off;
}

void AddrSpace::clock(int badVAddr) {
    printf("要用Clock置换咯！现在的内容是：\n");
    Print();
    printf("\n");
    unsigned int oldPage;
    int count = 0;  //用来计数，看看这一轮循环有没有能换的
    for (int i = 0; i < MemPages; ++i) {
        printf("第一次循环，当前指针：%d \n", p_vm);
        //        p_vm=i;//指向要被替换出的页
        if (pageTable[virtualMem[p_vm]].use == 0 &&
            pageTable[virtualMem[p_vm]].dirty == 0) {
            oldPage = pageTable[virtualMem[p_vm]].virtualPage;
            printf("第一次循环，找到的要替换的页是：%d \n", oldPage);
            break;  //找到了u=0.d=0的页
        }
        p_vm = (p_vm + 1) % MemPages;  //指向要被替换出的页,循环转圈
        count++;
    }
    //    printf("第一次循环没有找到（0，0）页。\n");
    if (count == MemPages) {  //说明没有u=0,d=0的页,重新开始循环
        count = 0;
        for (int i = 0; i < MemPages; i++) {
            //            p_vm=i;
            printf("第二次循环，当前指针：%d \n", p_vm);
            if (pageTable[virtualMem[p_vm]].use == 0 &&
                pageTable[virtualMem[p_vm]].dirty == 1) {
                oldPage = pageTable[virtualMem[p_vm]].virtualPage;
                printf("第二次循环，找到的要替换的页是：%d \n", oldPage);
                break;  //找到了u=0.d=1的页
            }
            //对于每个跳过的帧，把使用为置为0
            pageTable[virtualMem[p_vm]].use = 0;
            p_vm = (p_vm + 1) % MemPages;
            count++;
        }
    }
    //    printf("第二次循环没有找到（0，1）页。\n");
    if (count == MemPages) {  //不应该+1
        count = 0;
        for (int i = 0; i < MemPages; ++i) {
            //            p_vm = i;//指向要被替换出的页
            printf("第三次循环，当前指针：%d \n", p_vm);
            if (pageTable[virtualMem[p_vm]].use == 0 &&
                pageTable[virtualMem[p_vm]].dirty == 0) {
                oldPage = pageTable[virtualMem[p_vm]].virtualPage;
                printf("第三次循环，找到的要替换的页是：%d \n", oldPage);
                break;  //找到了u=0.d=0的页
            }
            p_vm = (p_vm + 1) % MemPages;
            count++;
        }
    }
    //    printf("第三次循环没有找到（0，0）页。\n");
    if (count == MemPages) {  //第三轮扫描（0,0）的失败了！找（0,1）的。
        for (int i = 0; i < MemPages;
             ++i)  //四轮扫描一定会找到合适的页面进行替换，所以不用count计数了。
        {
            //            p_vm=i;//指向要被替换出的页
            printf("第四次循环，当前指针：%d \n", p_vm);
            if (pageTable[virtualMem[p_vm]].use == 0 &&
                pageTable[virtualMem[p_vm]].dirty == 1) {
                oldPage = pageTable[virtualMem[p_vm]].virtualPage;
                printf("第四次循环，找到的要替换的页是：%d \n", oldPage);
                break;  //找到了u=0.d=1的页
            }
            p_vm = (p_vm + 1) % MemPages;
        }
    }

    unsigned int newPage;
    unsigned int temp;
    Translate(badVAddr, &newPage, &temp);
    printf("%d,%d\n", newPage, p_vm);
    ASSERT(newPage < numPages);
    virtualMem[p_vm] = newPage;
    p_vm = (p_vm + 1) % MemPages;  //后移指针
    printf("发生了页置换：%d换出，%d换入，交换完毕后如下：\n", oldPage,
           newPage);
    Swap(oldPage, newPage, badVAddr);
}

//交换页面
void AddrSpace::Swap(int oldPage, int newPage, int badVAddr) {
    WriteBack(oldPage, badVAddr);  //写回旧页
    pageTable[newPage].physicalPage =
        pageTable[oldPage].physicalPage;  //局部置换
    pageTable[oldPage].physicalPage = -1;
    pageTable[oldPage].valid = FALSE;

    pageTable[newPage].valid = TRUE;
    // pageTable[newPage].use = TRUE;
    // pageTable[newPage].dirty = FALSE;
    pageTable[newPage].use = 1;
    pageTable[newPage].dirty = 0;
    ReadIn(newPage, badVAddr);  //读入新页
    Print();                    //打印
}

//写回旧页
void AddrSpace::WriteBack(int oldPage, int badVAddr) {
    if (pageTable[oldPage].dirty) {
        int VN = badVAddr / PageSize;
        int PA = pageTable[p_vm].physicalPage * PageSize;
        char *buffer = new char[PageSize];
        for (int j = 0; j < PageSize; j++)
            buffer[j] = machine->mainMemory[PA + j];
        executable->WriteAt(buffer, PageSize, VN * PageSize + 40);
        delete[] buffer;
        pageTable[oldPage].dirty = FALSE;
    }
}

//读入新页
void AddrSpace::ReadIn(int newPage, int badVAddr) {
    int VN = badVAddr / PageSize;
    int PA = pageTable[p_vm].physicalPage * PageSize;
    executable->ReadAt(&(machine->mainMemory[PA]), PageSize,
                       VN * PageSize + 40);
}
