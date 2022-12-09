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

#include "copyright.h"
// #include "noff.h"
#include "machine.h"
#include "system.h"

// This makes VSCode happy (stop complaining about the absence of bzero)
#ifdef _WIN32
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)
extern FileSystem *fileSystem;
extern Machine *machine;
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

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

BitMap *AddrSpace::userMap = new BitMap(pnperp);

AddrSpace::AddrSpace(OpenFile *executable, char *filename) {
    // ------------------ Constructor ------------------
    bool flag = false;
    for (int i = 0; i < 128; i++) {
        if (!ThreadMap[i]) {
            ThreadMap[i] = 1;
            flag = true;
            spaceID = i;
            break;
        }
    }
    ASSERT(flag);

    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    // printf("open file %s\n", filename);
    this->filename = filename;

    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
           UserStackSize;  // we need to increase the size
                           // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
    ASSERT(numPages <= NumPhysPages);  // check we're not trying
                                       // to run anything too big --
                                       // at least until we have
                                       // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
          size);
    // first, set up the translation

    StackPages = divRoundUp(UserStackSize, PageSize);  // 用户栈页数
    pageTable = new TranslationEntry[numPages];

    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i; 
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = false;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;  // if the code segment was entirely on
                                        // a separate page, we could set its
                                        // pages to be read-only
    }

    bzero(machine->mainMemory, pnperp);
    fileSystem->Create("VMFile", size);
    OpenFile *vm = fileSystem->Open("VMFile");

    if (vm == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }

    char *__VM__ = new char[size];
    // 将用户程序的内容写入磁盘的中间过渡

    if (noffH.code.size > 0) {
        DEBUG('a', "\tCopying code segment, at 0x%x, size %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(__VM__[noffH.code.virtualAddr]), noffH.code.size,
                           noffH.code.inFileAddr);
        vm->WriteAt(&(__VM__[noffH.code.virtualAddr]), noffH.code.size,
                    noffH.code.virtualAddr * PageSize);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "\tCopying data segment, at 0x%x, size %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(__VM__[noffH.initData.virtualAddr]),
                           noffH.initData.size, noffH.initData.inFileAddr);
        vm->WriteAt(&(__VM__[noffH.initData.virtualAddr]), noffH.initData.size,
                    noffH.initData.virtualAddr * PageSize);
    }
    //
    delete vm;

    Print();
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
    delete[] pageTable;
    for (int i = 0; i < numPages; i++) {
        userMap->Clear(pageTable[i].physicalPage);
    }
    ThreadMap[spaceID] = 0;
    delete[] virtualMem;
}

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

void AddrSpace::Print() {
    printf("spaceID: %d ", spaceID);
    printf("page table dump: %d pages in total\n", numPages);
    printf(
        "======================================================================"
        "==\n");
    printf("\tVirtPage\tPhysPage\tValid\t\tUse\tDirty\n");
    for (int i = 0; i < numPages; i++) {
        printf("\t%d\t\t%d\t\t%d\t\t%d\t%d\n", pageTable[i].virtualPage,
               pageTable[i].physicalPage, pageTable[i].valid, pageTable[i].use,
               pageTable[i].dirty);
    }
    printf(
        "======================================================================"
        "==\n\n");
}

int AddrSpace::FIFO(int badVAddr) {
    printf("--------------- FIFO Algorithm ---------------\n");
    int temp = 0;
    if ((temp = userMap->Find()) != -1) {
        int newPage = badVAddr / PageSize;
        printf("%d页写入,不需要写出旧页.\n", newPage);
        virtualMem[p_vm] = newPage;
        advancePtr();
        pageTable[newPage].physicalPage = temp;
        OpenFile *vm = fileSystem->Open("VMFile");
        vm->ReadAt(
            &(machine->mainMemory[pageTable[newPage].physicalPage * PageSize]),
            PageSize, newPage * PageSize);
        delete vm;

        pageTable[newPage].valid = true;
        pageTable[newPage].use = true;
        pageTable[newPage].dirty = false;
        pageTable[newPage].readOnly = false;
        Print();
        return 0;
    } else {
        int oldVPN = virtualMem[p_vm];
        int newPage = badVAddr / PageSize;
        virtualMem[p_vm] = newPage;
        advancePtr();

        a = Swap(oldVPN, newPage);

        OpenFile *executable = fileSystem->Open("VMFile");
        if (executable == NULL) {
            printf("Unable to open filssse %s\n", filename);
            return 3;
        }
        executable->ReadAt(
            &(machine->mainMemory[pageTable[newPage].physicalPage * PageSize]),
            PageSize, newPage * PageSize);
        delete executable;
        Print();
        return a + 1;
    }
}

int AddrSpace::Swap(int oldVPN, int newPage) {
    int a = writeBack(oldVPN);

    pageTable[newPage].physicalPage = pageTable[oldVPN].physicalPage;
    printf("Swap out oldVPN: %d, Swap in newPage: %d (frame %d)\n", oldVPN,
           newPage, pageTable[oldVPN].physicalPage);
    pageTable[oldVPN].valid = FALSE;
    pageTable[newPage].physicalPage = pageTable[oldVPN].physicalPage;
    pageTable[newPage].valid = TRUE;
    pageTable[newPage].use = TRUE;
    pageTable[newPage].dirty = FALSE;
    return a;
}

// if dirty bit set to true, write back to disk
int AddrSpace::writeBack(int oldVPN) {
    if (pageTable[oldVPN].dirty) {
        OpenFile *executable = fileSystem->Open("VMFile");
        if (executable == NULL) {
            printf("Unable to open files %s\n", filename);
            return 0;
        }
        executable->WriteAt(
            &(machine->mainMemory[pageTable[oldVPN].physicalPage * PageSize]),
            PageSize, oldVPN * PageSize);
        delete executable;
        return 1;
    }
    return 0;
}

int AddrSpace::clock(int badVAddr) {
    printf("--------------- CLOCK Algorithm ---------------\n");
    int temp = 0;
    if ((temp = userMap->Find()) != -1) {
        // user map is not full
        int newPage = badVAddr / PageSize;
        printf("temp %d\n", temp);
        printf("%d页写入,不需要写出旧页\n", newPage);
        virtualMem[p_vm] = newPage;
        advancePtr();
        pageTable[newPage].physicalPage = temp;

        OpenFile *vm = fileSystem->Open("VMFile");

        vm->ReadAt(
            &(machine->mainMemory[pageTable[newPage].physicalPage * PageSize]),
            PageSize, newPage * PageSize);
        delete vm;

        pageTable[newPage].valid = true;
        pageTable[newPage].use = true;
        pageTable[newPage].dirty = false;
        pageTable[newPage].readOnly = false;
        Print();
        return 0;

    } else {
        int oldVPN;
        int count = 0;  // circle count
        // search from (0, 0)
        for (int i = 0; i < pnperp; ++i) {
            if (notUsednotDirty()) {
                oldVPN = ptrVPN();
                printf("第一轮，找到的要替换的页是：%d \n", oldVPN);
                break;
            }
            advancePtr();  // 指向要被替换出的页,循环转圈
            count++;
        }
        if (count == pnperp) {  // 2th
            count = 0;
            for (int i = 0; i < pnperp; i++) {
                if (notUsedbutDirty()) {
                    oldVPN = ptrVPN();
                    printf("第二轮，找到的要替换的页是：%d \n", oldVPN);
                    break;
                }

                pageTable[virtualMem[p_vm]].use = 0;
                advancePtr();
                count++;
            }
        }
        if (count == pnperp) {
            count = 0;
            for (int i = 0; i < pnperp; ++i) {
                if (notUsednotDirty()) {
                    oldVPN = ptrVPN();
                    printf("第三轮，找到的要替换的页是：%d \n", oldVPN);
                    break;
                }
                advancePtr();
                count++;
            }
        }
        if (count == pnperp) {
            for (int i = 0; i < pnperp; ++i) {
                if (notUsedbutDirty()) {
                    oldVPN = ptrVPN();
                    printf("第四轮，找到的要替换的页是：%d \n", oldVPN);
                    break;
                }
                advancePtr();
            }
        }

        int newPage = badVAddr / PageSize;
        ASSERT(newPage < numPages);
        virtualMem[p_vm] = newPage;
        advancePtr();  // moveback pointer

        a = Swap(oldVPN, newPage);

        OpenFile *executable = fileSystem->Open("VMFile");

        if (executable == NULL) {
            printf("Unable to open filssse %s\n", filename);
            return 3;
        }
        executable->ReadAt(
            &(machine->mainMemory[pageTable[newPage].physicalPage * PageSize]),
            PageSize, newPage * PageSize);
        delete executable;
        Print();
    }
    return 1 + a;
}