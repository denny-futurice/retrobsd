/*
 * System call results (sys/m68k/trap.c, D23): success returns u_rval in
 * d0 and clears the carry, an error returns the errno in d0 and sets it,
 * EJUSTRETURN leaves d0 and the carry alone.  No other register changes.
 */
#include "ttest.h"

int regs_in[14] = {                     /* d1-d7, a0-a6 */
    0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555,
    0x66666666, 0x77777777, 0x88888888, 0x99999999, 0xaaaaaaaa,
    0xbbbbbbbb, 0xcccccccc, 0xdddddddd, 0xeeeeeeee,
};
int regs_ok[15];                        /* d0-d7, a0-a6 */
u_int sp_ok;
int d0_err, d0_just;
short ccr_ok, ccr_err, ccr_just;

static void
sc_ok()
{
    u.u_rval = 0x12345678;
}

static void
sc_err()
{
    u.u_error = EBADF;
}

static void
sc_just()
{
    u.u_error = EJUSTRETURN;
}

const struct sysent sysent[] = {
    { 0, nosys }, { 0, sc_ok }, { 0, sc_err }, { 0, sc_just },
};
const int nsysent = 4;

extern char sysret_code[];
__asm__ (
"       .text\n"
"       .globl  sysret_code\n"
"sysret_code:\n"
"       move.l  usertop, %sp\n"
"       movem.l regs_in, %d1-%d7/%a0-%a6\n"
"       moveq   #1, %d0\n"
"       move.w  #1, %ccr\n"             /* carry set: success clears it */
"       trap    #0\n"
"       move.w  %ccr, ccr_ok\n"
"       movem.l %d0-%d7/%a0-%a6, regs_ok\n"
"       move.l  %sp, sp_ok\n"
"       moveq   #2, %d0\n"
"       trap    #0\n"
"       move.w  %ccr, ccr_err\n"
"       move.l  %d0, d0_err\n"
"       moveq   #3, %d0\n"
"       move.w  #1, %ccr\n"
"       trap    #0\n"
"       move.w  %ccr, ccr_just\n"
"       move.l  %d0, d0_just\n"
"       illegal\n");

void
got_signal(int sig)
{
    int i;

    check("signal", sig == SIGILL);
    check("success", regs_ok[0] == 0x12345678 && ! (ccr_ok & PSL_C));
    for (i = 0; i < 14; i++)
        check("registers", regs_ok[i + 1] == regs_in[i]);
    check("usp", sp_ok == usertop);
    check("error", d0_err == EBADF && (ccr_err & PSL_C));
    check("EJUSTRETURN", d0_just == 3 && (ccr_just & PSL_C));
    report("sysret");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) sysret_code, 0);
    return 0;
}
