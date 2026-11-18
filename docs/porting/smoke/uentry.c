/*
 * Process 1 entry (sys/m68k/startup.S): when main() returns, the code at
 * USER_DATA_START runs in user mode at IPL 0, on a user stack at
 * USER_DATA_END, with the kernel stack back at the top of u and every
 * register cleared.  The probe makes a system call so the trap frame shows
 * the state it ran with; main() leaves the registers and stack dirty.
 */
#include "ttest.h"

#define PROBESIZE 4                     /* moveq; trap */

extern char uentry_probe[], u_end[];
__asm__ (
"       .text\n"
"       .globl  uentry_probe\n"
"uentry_probe:\n"
"       moveq   #1, %d0\n"
"       trap    #0\n");

static void
probe_call()
{
    struct trapframe *tf = u.u_frame;
    int i, zero = 1;

    for (i = 1; i < 15; i++)
        if (tf->tf_regs[i] != 0)
            zero = 0;
    check("sr", tf->tf_sr == 0);
    check("pc", tf->tf_pc == USER_DATA_START + PROBESIZE);
    check("usp", tf->tf_sp == USER_DATA_END);
    check("d0", tf->tf_d0 == 1);
    check("registers", zero);
    check("kernel stack", tf->tf_format == 0 &&
        (char *) &tf->tf_sr + FMT0SIZE == u_end);
    report("uentry");
}

const struct sysent sysent[] = { { 0, nosys }, { 0, probe_call } };
const int nsysent = 2;

void
got_signal(int sig)
{
    printf("uentry: signal %d\n", sig);
}

void
setup()
{
    ttest_init();
    bcopy(uentry_probe, (caddr_t) USER_DATA_START, PROBESIZE);
}

/*
 * main() returns with every register dirty and the kernel stack 64 bytes
 * lower than it was called with, for startup.S to clean up.
 */
__asm__ (
"       .text\n"
"       .globl  main\n"
"main:\n"
"       jsr     setup\n"
"       move.l  (%sp), -64(%sp)\n"
"       lea     -64(%sp), %sp\n"
"       moveq   #-1, %d0\n"
"       move.l  %d0, %d1\n"
"       move.l  %d0, %d2\n"
"       move.l  %d0, %d3\n"
"       move.l  %d0, %d4\n"
"       move.l  %d0, %d5\n"
"       move.l  %d0, %d6\n"
"       move.l  %d0, %d7\n"
"       movea.l %d0, %a0\n"
"       movea.l %d0, %a1\n"
"       movea.l %d0, %a2\n"
"       movea.l %d0, %a3\n"
"       movea.l %d0, %a4\n"
"       movea.l %d0, %a5\n"
"       movea.l %d0, %a6\n"
"       rts\n");
