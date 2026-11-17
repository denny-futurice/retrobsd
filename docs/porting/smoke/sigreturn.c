/*
 * Signal round trip in user mode (sys/m68k/signal.c, D31): a handler
 * entered by sendsig returns through a trampoline and sigreturn; the
 * registers, PC, mask and user SR bits come from its sigcontext.
 */
#include "ttest.h"

#define SYS_sigreturn   103             /* include/syscall.h */

const struct sysent sysent[SYS_sigreturn + 1] = {
    [0] = { 0, nosys },
    [SYS_sigreturn] = { 1, sigreturn },
};
const int nsysent = SYS_sigreturn + 1;

int before[15] = {
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
};
int after[15], sp_after;
short ccr_after;

extern char sigret_code[], tramp[];
__asm__ (
"       .text\n"
"       .globl  sigret_code, tramp\n"
"sigret_code:\n"
"       move.l  usertop, %sp\n"
"       movem.l before, %d0-%d7/%a0-%a6\n"
"       illegal\n"                      /* the handler skips this */
"       move.w  %ccr, ccr_after\n"
"       movem.l %d0-%d7/%a0-%a6, after\n"
"       move.l  %sp, sp_after\n"
"       illegal\n"
"tramp:\n"                              /* sp at sf_signum */
"       pea     12(%sp)\n"              /* &sf_sc */
"       clr.l   -(%sp)\n"               /* return address slot */
"       moveq   #103, %d0\n"
"       trap    #0\n"
"       trap    #1\n");                 /* sigreturn failed */

static int h_sig, h_code;
static struct sigcontext *h_scp;

/* The user handler. */
static void
handler(int sig, int code, struct sigcontext *scp)
{
    h_sig = sig;
    h_code = code;
    h_scp = scp;
    scp->sc_pc += 2;
    scp->sc_ps = PSL_S | PSL_IPL7 | 0x04;       /* only Z may stay */
    scp->sc_regs[2] = 0xdead;
    scp->sc_mask = 0x10 | sigmask(SIGKILL);
}

void
got_signal(int sig)
{
    static int n;
    int i, regs;

    check("signal", sig == SIGILL);
    if (n++ == 0) {
        u.u_code = 55;
        sendsig((sig_t) handler, sig, 0x10);
        return;
    }
    check("handler args", h_sig == SIGILL && h_code == 55 &&
        (u_int) h_scp > USER_DATA_END - 32 - sizeof(*h_scp));
    regs = sp_after == usertop;
    for (i = 0; i < 15; i++)
        regs &= after[i] == (i == 2 ? 0xdead : before[i]);
    check("registers", regs);
    check("ccr", (ccr_after & PSL_CCR) == 0x04);
    check("sr", (u.u_frame->tf_sr & (PSL_S | PSL_IPL)) == 0);
    check("mask", u.u_procp->p_sigmask == 0x10);
    report("sigreturn");
}

int
main()
{
    ttest_init();
    u.u_sigtramp = (u_int) tramp;
    enter_user((void (*)(void)) sigret_code, 0);
    return 0;
}
