/*
 * User-mode illegal instruction (sys/m68k/locore.S, trap.c): SIGILL, with
 * u.u_frame at a user-mode trap frame holding the PC of the instruction
 * and the user sp.
 */
#include "ttest.h"

extern char uill_code[];
__asm__ (
"       .text\n"
"       .globl  uill_code\n"
"uill_code:\n"
"       illegal\n");

void
got_signal(int sig)
{
    struct trapframe *tf = u.u_frame;

    check("signal", sig == SIGILL);
    check("vector", tf->tf_vector == 4 * 4);
    check("pc", tf->tf_pc == (u_int) uill_code);
    check("user mode", USERMODE(tf->tf_sr));
    check("usp", tf->tf_sp == (int) (ustack + USTACK));
    report("uill");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) uill_code, 0);
    return 0;
}
