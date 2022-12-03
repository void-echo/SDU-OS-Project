// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"
#include "copyright.h"
#include "machine.h"
#include "syscall.h"
#include "system.h"
void AdvancePC() {
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg) + 4);
    machine->WriteRegister(PCReg, machine->ReadRegister(PCReg) + 4);
}
void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    } else if ((which == SyscallException) && (type == SC_Exec)) {
        printf("Execute system call of Exec()\n");
        char filename[50];
        int addr = machine->ReadRegister(4);
        int i = 0;

        //        我们的参数不能以直接转换成指针的形式从内存中读取，
        //        原因是这里的内存地址实际上是虚拟机的内存，
        //        而转换成指针形式的内存地址是真实机器中的地址。
        //        所以读取要用machine->ReadMem(int addr,int size,int
        //        *value)方法。 而读取字符串，则需要循环至读取到\0为止。
        machine->ReadMem(addr + i, 1,
                         (int *)&filename[i]);  //读取filename，要先读出来
        while (filename[i++] != '\0') {
            machine->ReadMem(addr + i, 1, (int *)&filename[i]);
        }
        printf("Exec(%s):\n", filename);

        interrupt->Exec(filename);
        AdvancePC();
    } else if (which == PageFaultException) {  //处理缺页异常

        interrupt->PageFault();
    } else {
        interrupt->Halt();
    }
}
