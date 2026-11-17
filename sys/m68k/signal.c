/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/frame.h>

/*
 * The signal frame on the user stack (D31), lowest address first: the
 * handler is called as handler(sig, code, scp) and returns to the
 * trampoline at u.u_sigtramp, with sp at sf_signum.  The trampoline
 * pushes the address of sf_sc (sp + 12) and a dummy return address and
 * calls sigreturn, then exit if that fails.
 */
struct sigframe {
    int     sf_ra;                  /* trampoline address */
    int     sf_signum;
    int     sf_code;
    struct  sigcontext *sf_scp;
    struct  sigcontext sf_sc;
};

/* Version byte of this FPU's fsave frames, from the last non-null save. */
static u_char fpuversion;

/*
 * Send an interrupt to process: save the user state in a signal frame on
 * the user stack (or the alternate signal stack) and enter the handler
 * p with a reset FPU.  The frame is built here and copied out (D31).
 */
void
sendsig(sig_t p, int sig, long mask)
{
    struct trapframe *tf = u.u_frame;
    struct sigframe sf, *sfp;
    int oonstack;

    oonstack = u.u_sigstk.ss_flags & SA_ONSTACK;
    if ((u.u_psflags & SAS_ALTSTACK) && ! oonstack &&
        (u.u_sigonstack & sigmask(sig))) {
        sfp = (struct sigframe *) (u.u_sigstk.ss_base + u.u_sigstk.ss_size);
        u.u_sigstk.ss_flags |= SA_ONSTACK;
    } else
        sfp = (struct sigframe *) tf->tf_sp;
    sfp--;
    if (! (u.u_sigstk.ss_flags & SA_ONSTACK)) {
        if ((caddr_t) sfp < (caddr_t) u.u_procp->p_daddr + u.u_dsize) {
            /*
             * Process has trashed its stack; give it an illegal
             * instruction violation to halt it in its tracks.
             */
            fatalsig(SIGILL);
            return;
        }
        if (u.u_procp->p_ssize < USER_DATA_END - (u_int) sfp) {
            u.u_procp->p_ssize = USER_DATA_END - (u_int) sfp;
            u.u_procp->p_saddr = (u_int) sfp;
            u.u_ssize = u.u_procp->p_ssize;
        }
    }

    sf.sf_ra = u.u_sigtramp;
    sf.sf_signum = sig;
    sf.sf_code = u.u_code;
    sf.sf_scp = &sfp->sf_sc;
    sf.sf_sc.sc_onstack = oonstack;
    sf.sf_sc.sc_mask = mask;
    sf.sf_sc.sc_flags = 0;
    bcopy(tf->tf_regs, sf.sf_sc.sc_regs, sizeof(sf.sf_sc.sc_regs));
    sf.sf_sc.sc_pc = tf->tf_pc;
    sf.sf_sc.sc_ps = tf->tf_sr;
    savefpu();
    if (u.u_fpstate.fps_frame[0] != 0)
        fpuversion = u.u_fpstate.fps_frame[0];
    sf.sf_sc.sc_fpstate = u.u_fpstate;
    if (copyout((caddr_t) &sf, (caddr_t) sfp, sizeof(sf)) != 0) {
        fatalsig(SIGILL);
        return;
    }

    /* The handler starts with a reset FPU, as after exec (D30). */
    bzero(&u.u_fpstate, sizeof(u.u_fpstate));
    restorefpu();

    /*
     * rte must not resume an FPU instruction (format 9) in the handler;
     * enter it from a format 0 frame, and without tracing.
     */
    if (tf->tf_format != 0)
        trimframe(tf);
    tf->tf_sp = (int) sfp;
    tf->tf_pc = (int) p;
    tf->tf_sr &= ~(PSL_T | PSL_T0);
}

/* A frame frestore accepts: null, or this FPU's idle or busy frame. */
static int
goodfpframe(const u_char *frame)
{
    if (frame[0] == 0)
        return 1;
    if (frame[0] != fpuversion)
        return 0;
    switch (frame[1]) {
    case FPF_IDLE81:
    case FPF_IDLE82:
    case FPF_BUSY81:
    case FPF_BUSY82:
        return 1;
    }
    return 0;
}

/*
 * System call to clean up state after a signal has been taken (D31):
 * reset the signal mask and stack state, the registers, PC and FPU from
 * the context left by sendsig, which the handler may have changed.  SR
 * keeps only the user bits (D22); an odd PC, unknown flags or an FPU
 * frame frestore would reject are refused.
 */
void
sigreturn()
{
    struct a {
        struct sigcontext *scp;
    } *uap = (struct a *) u.u_arg;
    struct trapframe *tf = u.u_frame;
    struct sigcontext sc;

    u.u_error = copyin((caddr_t) uap->scp, (caddr_t) &sc, sizeof(sc));
    if (u.u_error)
        return;
    if (sc.sc_flags != 0 || (sc.sc_pc & 1) ||
        ! goodfpframe(sc.sc_fpstate.fps_frame)) {
        u.u_error = EINVAL;
        return;
    }
    u.u_error = EJUSTRETURN;
    if (sc.sc_onstack & SA_ONSTACK)
        u.u_sigstk.ss_flags |= SA_ONSTACK;
    else
        u.u_sigstk.ss_flags &= ~SA_ONSTACK;
    u.u_procp->p_sigmask = sc.sc_mask & ~sigcantmask;

    bcopy(sc.sc_regs, tf->tf_regs, sizeof(tf->tf_regs));
    tf->tf_pc = sc.sc_pc;
    tf->tf_sr = sc.sc_ps & PSL_USERSR;
    u.u_fpstate = sc.sc_fpstate;
    restorefpu();
}
