/*
 * Nonexistent system call (sys/m68k/trap.c): a number out of range goes
 * to sysent[0], nosys(), which posts SIGSYS; with SIGSYS ignored the
 * call also fails with EINVAL.
 */
#include "ttest.h"
#include <sys/signalvar.h>

static int sigsys;
int d0_dfl, d0_ign;
short ccr_dfl, ccr_ign;

extern char nosys_code[];
__asm__ (
"       .text\n"
"       .globl  nosys_code\n"
"nosys_code:\n"
"       move.l  usertop, %sp\n"
"       moveq   #-1, %d0\n"
"       trap    #0\n"
"       move.w  %ccr, ccr_dfl\n"
"       move.l  %d0, d0_dfl\n"
"       move.l  #1000, %d0\n"
"       trap    #0\n"
"       move.w  %ccr, ccr_ign\n"
"       move.l  %d0, d0_ign\n"
"       illegal\n");

void
got_signal(int sig)
{
    if (sig == SIGSYS) {
        sigsys++;
        u.u_signal[SIGSYS] = SIG_IGN;   /* for the second call */
        return;
    }
    check("signal", sig == SIGILL);
    check("SIGSYS", sigsys == 2);
    check("default", d0_dfl == 0 && ! (ccr_dfl & PSL_C));
    check("ignored", d0_ign == EINVAL && (ccr_ign & PSL_C));
    report("nosys");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) nosys_code, 0);
    return 0;
}
