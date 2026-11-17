/*
 * Placeholders for kernel symbols that M3 implements (decision D20), so the
 * M2 kernel links.  Every function panics with "<name> not implemented";
 * none is reached on the way to the M2 finish line, the "No root
 * filesystem found!" panic.  M3 removes each entry as the real code
 * arrives, and then this file.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/exec.h>
#include <sys/signal.h>
#include <sys/ptrace.h>

#define NOTYET(name)    panic(#name " not implemented")

/*
 * User code for process 1, which execs /sbin/init: startup.S on PIC32.
 * Empty here; process 1 is not started without a root filesystem.
 */
__asm__ (
"       .section .rodata\n"
"       .globl  icode, initflags, icodeend\n"
"icode:\n"
"initflags:\n"
"       .byte   0\n"
"icodeend:\n"
"       .previous\n");

/*
 * Signals: signal.c on PIC32.
 */
void
sendsig(sig_t p, int sig, long mask)
{
    NOTYET(sendsig);
}

void
sigreturn()
{
    NOTYET(sigreturn);
}
