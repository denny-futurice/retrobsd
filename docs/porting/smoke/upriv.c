/*
 * User-mode privileged instruction (sys/m68k/locore.S, trap.c): a move to
 * SR is a privilege violation and gives SIGILL.
 */
#include "ttest.h"

extern char upriv_code[];
__asm__ (
"       .text\n"
"       .globl  upriv_code\n"
"upriv_code:\n"
"       move.w  #0x2700, %sr\n");

void
got_signal(int sig)
{
    struct trapframe *tf = u.u_frame;

    check("signal", sig == SIGILL);
    check("vector", tf->tf_vector == 8 * 4);
    check("pc", tf->tf_pc == (u_int) upriv_code);
    report("upriv");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) upriv_code, 0);
    return 0;
}
