/*
 * Bad signal state (sys/m68k/signal.c, D31): sigreturn refuses a context
 * outside the user region (EFAULT) and unknown flags, an odd PC or an FPU
 * frame frestore would reject (EINVAL), leaving the trap frame alone; a
 * signal frame that would run into the data gives SIGILL.
 */
#include "ttest.h"

static int signo;

void
got_signal(int sig)
{
    signo = sig;
}

static struct trapframe tf;

static int
try(struct sigcontext *scp)
{
    u.u_arg[0] = (int) scp;
    u.u_error = 0;
    sigreturn();
    return u.u_error;
}

int
main()
{
    struct sigcontext *sc = (struct sigcontext *) (USER_DATA_START + 0x100);

    ttest_init();
    u.u_frame = &tf;
    bzero(sc, sizeof(*sc));
    sc->sc_pc = USER_DATA_START + 0x200;
    check("good", try(sc) == EJUSTRETURN && tf.tf_pc == sc->sc_pc);

    tf.tf_pc = 0x12346;
    check("kernel", try((struct sigcontext *) 0x1000) == EFAULT);
    check("past end", try((struct sigcontext *) (USER_DATA_END - 16)) == EFAULT);
    sc->sc_flags = 1;
    check("flags", try(sc) == EINVAL);
    sc->sc_flags = 0;
    sc->sc_pc |= 1;
    check("odd pc", try(sc) == EINVAL);
    sc->sc_pc &= ~1;
    sc->sc_fpstate.fps_frame[0] = 0x1f;         /* no FPU frame seen */
    sc->sc_fpstate.fps_frame[1] = FPF_IDLE82;
    check("fpu frame", try(sc) == EINVAL);
    check("frame kept", tf.tf_pc == 0x12346);

    u.u_procp->p_daddr = USER_DATA_START;
    u.u_dsize = USER_DATA_SIZE - 64;
    tf.tf_sp = USER_DATA_END - 16;
    sendsig((sig_t) (USER_DATA_START + 0x500), SIGINT, 0);
    check("overflow", signo == SIGILL && tf.tf_sp == USER_DATA_END - 16 &&
        tf.tf_pc == 0x12346);
    report("sigfault");
}
