/*
 * User-mode bus error (sys/m68k/locore.S, trap.c), on a hand-built format
 * B frame since QEMU's 68030 raises none (D25): SIGBUS with the fault
 * address in u.u_code, and the frame is trimmed to format 0 (D26), so
 * rte returns to the frame's PC with the kernel stack back where it was.
 * The code there does a trap #1, whose SIGILL ends the test.
 */
#include "ttest.h"

extern char buserr_resume[];
__asm__ (
"       .text\n"
"       .globl  buserr_resume\n"
"buserr_resume:\n"
"       trap    #1\n");

#define FAULTADDR       0x00dead00

static int nsig;

void
got_signal(int sig)
{
    struct trapframe *tf = u.u_frame;

    if (nsig++ == 0) {
        check("SIGBUS", sig == SIGBUS);
        check("fault address", u.u_code == FAULTADDR);
        return;                         /* back to trap() and rte */
    }
    check("second signal", sig == SIGILL);
    check("trap #1", tf->tf_vector == 33 * 4 && tf->tf_format == 0);
    check("resumed", tf->tf_pc == (u_int) buserr_resume + 2);
    check("kernel sp", (u_int) &tf->tf_sr + FMT0SIZE == fake_ssp);
    check("usp", tf->tf_sp == (int) (ustack + USTACK));
    report("buserr");
}

int
main()
{
    static struct hwframe f;

    ttest_init();
    f.sr = 0;                           /* user mode */
    f.pc = (u_int) buserr_resume;
    f.fmtvec = 0xB000 | 2 * 4;
    f.u.fB.f_ssw = SSW_DF | SSW_RW;
    f.u.fB.f_dcfa = FAULTADDR;
    fake_trap(&f, FMTBSIZE);
    return 0;
}
