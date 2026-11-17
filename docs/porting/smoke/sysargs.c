/*
 * System call arguments (sys/m68k/trap.c, D23): trap #0 copies sy_narg
 * longs from usp + 4 into u_arg, and fails with EFAULT, without calling
 * the handler, if they run past the end of the user region.
 */
#include "ttest.h"

static int args[6], calls;
int d0_ok, d0_fault;
short ccr_ok, ccr_fault;

static void
sc_args()
{
    int i;

    calls++;
    for (i = 0; i < 6; i++)
        args[i] = u.u_arg[i];
    u.u_rval = 0x1234;
}

const struct sysent sysent[] = { { 0, nosys }, { 6, sc_args } };
const int nsysent = 2;

extern char sysargs_code[];
__asm__ (
"       .text\n"
"       .globl  sysargs_code\n"
"sysargs_code:\n"
"       move.l  usertop, %sp\n"
"       move.l  #6, -(%sp)\n"
"       move.l  #5, -(%sp)\n"
"       move.l  #4, -(%sp)\n"
"       move.l  #3, -(%sp)\n"
"       move.l  #2, -(%sp)\n"
"       move.l  #1, -(%sp)\n"
"       bsr.s   1f\n"
"       move.w  %ccr, ccr_ok\n"
"       move.l  %d0, d0_ok\n"
"       move.l  usertop, %sp\n"         /* arguments from usertop - 4 on */
"       subq.l  #8, %sp\n"
"       bsr.s   1f\n"
"       move.w  %ccr, ccr_fault\n"
"       move.l  %d0, d0_fault\n"
"       illegal\n"
"1:     moveq   #1, %d0\n"
"       trap    #0\n"
"       rts\n");

void
got_signal(int sig)
{
    int i;

    check("signal", sig == SIGILL);
    check("calls", calls == 1);
    for (i = 0; i < 6; i++)
        check("argument", args[i] == i + 1);
    check("result", d0_ok == 0x1234 && ! (ccr_ok & PSL_C));
    check("EFAULT", d0_fault == EFAULT && (ccr_fault & PSL_C));
    report("sysargs");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) sysargs_code, 0);
    return 0;
}
