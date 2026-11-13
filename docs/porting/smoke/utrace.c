/*
 * User-mode trace (sys/m68k/locore.S, trap.c): with the T bit set, the
 * first user instruction raises a format 2 trace exception; trap() gives
 * SIGTRAP and clears T in the saved SR.
 */
#include "ttest.h"

extern char utrace_code[], utrace_next[];
__asm__ (
"       .text\n"
"       .globl  utrace_code, utrace_next\n"
"utrace_code:\n"
"       nop\n"
"utrace_next:\n"
"       nop\n");

void
got_signal(int sig)
{
    struct trapframe *tf = u.u_frame;

    check("signal", sig == SIGTRAP);
    check("vector", tf->tf_vector == 9 * 4 && tf->tf_format == 2);
    check("pc", tf->tf_pc == (u_int) utrace_next);
    check("iaddr", tf->F_u.F_fmt2.f_iaddr == (u_int) utrace_code);
    check("T cleared", (tf->tf_sr & PSL_T) == 0);
    report("utrace");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) utrace_code, PSL_T);
    return 0;
}
