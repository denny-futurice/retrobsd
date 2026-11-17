/*
 * System call restart (sys/m68k/trap.c, D23): ERESTART backs the PC up
 * over trap #0, with the number still in d0, so the call runs again.
 */
#include "ttest.h"

static int calls;
int d0_res;
short ccr_res;

static void
sc_restart()
{
    if (++calls == 1)
        u.u_error = ERESTART;
    else
        u.u_rval = calls;
}

const struct sysent sysent[] = { { 0, nosys }, { 0, sc_restart } };
const int nsysent = 2;

extern char sysrestart_code[];
__asm__ (
"       .text\n"
"       .globl  sysrestart_code\n"
"sysrestart_code:\n"
"       move.l  usertop, %sp\n"
"       moveq   #1, %d0\n"
"       trap    #0\n"
"       move.w  %ccr, ccr_res\n"
"       move.l  %d0, d0_res\n"
"       illegal\n");

void
got_signal(int sig)
{
    check("signal", sig == SIGILL);
    check("calls", calls == 2);
    check("result", d0_res == 2 && ! (ccr_res & PSL_C));
    report("sysrestart");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) sysrestart_code, 0);
    return 0;
}
