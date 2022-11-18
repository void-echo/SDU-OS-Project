/* exec.c
 *	Simple program to running another user program.
 */

#include "syscall.h"

int
main()
{
    SpaceId pid;
    // PrintInt(12345);
    pid = Exec("../test/halt.noff");
    Halt();
    /* not reached */
}
