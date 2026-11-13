/*
 * User-mode FPU exception (sys/m68k/trap.c), on a hand-built frame for
 * vector 50, FPU divide by zero, since QEMU raises none (D25): SIGFPE,
 * with the FPSR in u.u_code.  The frame goes straight to trapentry, not
 * through fpfault, whose fsave QEMU's 68030 does not accept.
 */
#include "ttest.h"

#define FPSR    0x00000410              /* DZ exception and accrued bits */

void
got_signal(int sig)
{
    struct trapframe *tf = u.u_frame;

    check("signal", sig == SIGFPE);
    check("vector", tf->tf_vector == 50 * 4);
    check("fpsr", u.u_code == FPSR);
    report("ufpe");
}

int
main()
{
    static struct hwframe f;

    ttest_init();
    __asm__ volatile ("fmove.l %0, %%fpsr" : : "d" (FPSR));
    f.sr = 0;
    f.pc = (u_int) main;
    f.fmtvec = 50 * 4;
    fake_trap(&f, FMT0SIZE);
    return 0;
}
