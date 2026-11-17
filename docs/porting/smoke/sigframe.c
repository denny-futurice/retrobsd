/*
 * Signal frame (sys/m68k/signal.c sendsig(), D31): the handler gets
 * sig, code and scp on the stack below the sigcontext, returns to the
 * trampoline, and starts without tracing from a format 0 frame.
 */
#include "ttest.h"

void
got_signal(int sig)
{
    check("no signal", 0);
}

static struct trapframe tf;

int
main()
{
    u_int oldsp = USER_DATA_END - 16;
    struct sigcontext *sc;
    int *sp, i, regs;

    ttest_init();
    u.u_frame = &tf;
    for (i = 0; i < 15; i++)
        tf.tf_regs[i] = 0x01010101 * (i + 1);
    tf.tf_sp = oldsp;
    tf.tf_pc = USER_DATA_START + 0x200;
    tf.tf_sr = PSL_T | 0x15;
    tf.tf_format = 9;                   /* FPU mid-instruction */
    u.u_sigtramp = USER_DATA_START + 0x400;
    u.u_code = 77;
    sendsig((sig_t) (USER_DATA_START + 0x500), SIGINT, 0x1234);

    sp = (int *) tf.tf_sp;
    sc = (struct sigcontext *) (sp + 4);
    check("sp", (u_int) sp == oldsp - 4 * sizeof(int) - sizeof(*sc));
    check("return", sp[0] == USER_DATA_START + 0x400);
    check("args", sp[1] == SIGINT && sp[2] == 77 && sp[3] == (int) sc);
    check("context", sc->sc_mask == 0x1234 && sc->sc_onstack == 0 &&
        sc->sc_flags == 0);
    regs = sc->sc_sp == oldsp;
    for (i = 0; i < 15; i++)
        regs &= sc->sc_regs[i] == 0x01010101 * (i + 1);
    check("registers", regs);
    check("pc sr", sc->sc_pc == USER_DATA_START + 0x200 &&
        sc->sc_ps == (PSL_T | 0x15));
    check("handler", tf.tf_pc == USER_DATA_START + 0x500 && tf.tf_sr == 0x15);
    check("format", tf.tf_format == 0 &&
        tf.tf_stackadj == FMT9SIZE - FMT0SIZE);
    check("ssize", u.u_procp->p_ssize == USER_DATA_END - (u_int) sp);
    report("sigframe");
}
