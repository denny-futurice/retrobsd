/*
 * User-mode integer divide by zero (sys/m68k/locore.S, trap.c): SIGFPE,
 * from a format 2 frame that holds the address of the divide.
 */
#include "ttest.h"

extern char udivz_code[], udivz_div[];
__asm__ (
"       .text\n"
"       .globl  udivz_code, udivz_div\n"
"udivz_code:\n"
"       moveq   #0, %d1\n"
"udivz_div:\n"
"       divu.w  %d1, %d0\n");

void
got_signal(int sig)
{
    struct trapframe *tf = u.u_frame;

    check("signal", sig == SIGFPE);
    check("vector", tf->tf_vector == 5 * 4 && tf->tf_format == 2);
    check("iaddr", tf->F_u.F_fmt2.f_iaddr == (u_int) udivz_div);
    report("udivz");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) udivz_code, 0);
    return 0;
}
