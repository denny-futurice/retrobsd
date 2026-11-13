/*
 * Kernel-mode exception (sys/m68k/locore.S, trap.c): an illegal
 * instruction in supervisor mode dumps the registers and panics.
 */
#include "ttest.h"

void
got_signal(int sig)
{
    kputs("ktrap FAIL signal\n");
}

int
main()
{
    ttest_init();
    __asm__ volatile ("illegal");
    kputs("ktrap FAIL no trap\n");
    return 0;
}
