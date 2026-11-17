/*
 * FPU state across a signal (sys/m68k/signal.c, D31): sendsig saves it
 * in the sigcontext and gives the handler a reset FPU; sigreturn puts
 * the saved state back.
 */
#include "ttest.h"

#define SYS_sigreturn   103             /* include/syscall.h */

const struct sysent sysent[SYS_sigreturn + 1] = {
    [0] = { 0, nosys },
    [SYS_sigreturn] = { 1, sigreturn },
};
const int nsysent = SYS_sigreturn + 1;

double onehalf = 1.5, two = 2.0, junk = -7.0;
u_int fp_after[8 * 3], fpcr_after;

extern char sigfpu_code[], tramp[];
__asm__ (
"       .text\n"
"       .globl  sigfpu_code, tramp\n"
"sigfpu_code:\n"
"       move.l  usertop, %sp\n"
"       fmove.d onehalf, %fp0\n"
"       fmove.d two, %fp7\n"
"       fmove.l #0x30, %fpcr\n"         /* round toward +infinity */
"       illegal\n"                      /* the handler skips this */
"       fmovem.x %fp0-%fp7, fp_after\n"
"       fmove.l %fpcr, fpcr_after\n"
"       illegal\n"
"tramp:\n"
"       pea     12(%sp)\n"
"       clr.l   -(%sp)\n"
"       moveq   #103, %d0\n"
"       trap    #0\n"
"       trap    #1\n");

static u_int h_fp0[3], h_fpcr;
static int h_saved;

/* 1.5 and 2.0 in the 96-bit extended format. */
static const u_int x15[3] = { 0x3fff0000, 0xc0000000, 0 };
static const u_int x20[3] = { 0x40000000, 0x80000000, 0 };

static int
xeq(const u_int *a, const u_int *b)
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

/* The user handler: look at the FPU, then dirty it. */
static void
handler(int sig, int code, struct sigcontext *scp)
{
    const struct fpstate *fs = &scp->sc_fpstate;

    __asm__ volatile (
        "fmove.x %%fp0, %0\n"
        "fmove.l %%fpcr, %1"
        : "=m" (h_fp0), "=d" (h_fpcr));
    h_saved = xeq(&fs->fps_regs[0], x15) && xeq(&fs->fps_regs[7 * 3], x20) &&
        fs->fps_fpcr == 0x30;
    __asm__ volatile (
        "fmove.d %0, %%fp0\n"
        "fmove.d %0, %%fp1\n"
        "fmove.l #0x10, %%fpcr"
        : : "m" (junk) : "fp0", "fp1");
    scp->sc_pc += 2;
}

void
got_signal(int sig)
{
    static int n;

    check("signal", sig == SIGILL);
    if (n++ == 0) {
        sendsig((sig_t) handler, sig, 0);
        return;
    }
    check("saved", h_saved);
    check("reset", h_fp0[0] == 0 && h_fp0[1] == 0 && h_fp0[2] == 0 &&
        h_fpcr == 0);
    check("restored", xeq(&fp_after[0], x15) && xeq(&fp_after[7 * 3], x20) &&
        fpcr_after == 0x30);
    report("sigfpu");
}

int
main()
{
    ttest_init();
    u.u_sigtramp = (u_int) tramp;
    enter_user((void (*)(void)) sigfpu_code, 0);
    return 0;
}
