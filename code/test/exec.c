/* exec.c
 *	Simple program to running another user program.
 */

#include "syscall.h"

int main() {
    SpaceId pid;
    PrintInt(12345);
    pid = Exec("../test/halt.noff");
    PrintInt(114514);   // This line should not be printed
    Halt();
    /* not reached */
}
