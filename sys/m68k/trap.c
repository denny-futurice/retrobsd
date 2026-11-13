/*
 * Exception, trap and interrupt handling for the 68030 (exception.c on
 * PIC32).  locore.S builds a struct trapframe and calls trap().
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/vmmeter.h>
#include <machine/frame.h>

/*
 * locore.S uses these offsets.  C99 has no _Static_assert: a false
 * condition gives a negative array size, a compile error.
 */
#define CTASSERT(name, cond)    typedef char name[(cond) ? 1 : -1]

CTASSERT(tf_sp_offset, offsetof(struct trapframe, tf_regs) + 15 * sizeof(int) == 60);
CTASSERT(tf_stackadj_offset, offsetof(struct trapframe, tf_stackadj) == 66);
CTASSERT(tf_sr_offset, offsetof(struct trapframe, tf_sr) == 68);
CTASSERT(F_u_offset, offsetof(struct trapframe, F_u) == 68 + FMT0SIZE);

/* Vector numbers. */
#define T_BUSERR        2
#define T_ADDRERR       3
#define T_ILLINST       4
#define T_ZERODIV       5
#define T_CHK           6
#define T_TRAPV         7
#define T_PRIVINST      8
#define T_TRACE         9
#define T_LINEA         10
#define T_LINEF         11
#define T_COPERR        13
#define T_UNINITINT     15
#define T_SPURIOUS      24
#define T_AUTOVEC7      31
#define T_TRAP0         32
#define T_TRAP15        47
#define T_FPFIRST       48              /* FPU branch on unordered */
#define T_FPLAST        54              /* FPU signaling NaN */
#define T_USERVEC       64              /* first vectored interrupt */

#define ISINTR(v)       ((v) == T_UNINITINT || \
                         ((v) >= T_SPURIOUS && (v) <= T_AUTOVEC7) || \
                         (v) >= T_USERVEC)

static const char *const trapnames[] = {
    [T_BUSERR]      = "bus error",
    [T_ADDRERR]     = "address error",
    [T_ILLINST]     = "illegal instruction",
    [T_ZERODIV]     = "zero divide",
    [T_CHK]         = "CHK",
    [T_TRAPV]       = "TRAPV",
    [T_PRIVINST]    = "privilege violation",
    [T_TRACE]       = "trace",
    [T_LINEA]       = "line A",
    [T_LINEF]       = "line F",
    [T_COPERR]      = "coprocessor protocol violation",
    [14]            = "format error",
    [T_UNINITINT]   = "uninitialized interrupt",
    [T_SPURIOUS]    = "spurious interrupt",
    [48]            = "FPU branch on unordered",
    [49]            = "FPU inexact result",
    [50]            = "FPU divide by zero",
    [51]            = "FPU underflow",
    [52]            = "FPU operand error",
    [53]            = "FPU overflow",
    [54]            = "FPU signaling NaN",
    [56]            = "MMU configuration error",
};

/* Hardware frame size by format; 0 for formats the 68030 does not use. */
static const u_char framesize[16] = {
    [0x0] = FMT0SIZE, [0x1] = FMT1SIZE, [0x2] = FMT2SIZE,
    [0x9] = FMT9SIZE, [0xA] = FMTASIZE, [0xB] = FMTBSIZE,
};

void trap(struct trapframe *tf);
void fpfault_clear(u_char *state);
extern char trapentry[], fpfault[];

static const char *
trapname(int vec)
{
    if (vec < (int) (sizeof(trapnames) / sizeof(trapnames[0])) && trapnames[vec])
        return trapnames[vec];
    if (vec >= T_TRAP0 && vec <= T_TRAP15)
        return "trap instruction";
    if (vec > T_SPURIOUS && vec <= T_AUTOVEC7)
        return "autovectored interrupt";
    if (vec >= T_USERVEC)
        return "vectored interrupt";
    return "unknown";
}

/* Fault address of a bus or address error, from a format A or B frame. */
static u_int
faultaddr(struct trapframe *tf)
{
    if (tf->tf_format == 0xA || tf->tf_format == 0xB)
        return tf->F_u.F_fmtA.f_dcfa;
    return 0;
}

static void
dumpregs(struct trapframe *tf)
{
    int vec = tf->tf_vector >> 2;
    int i;

    printf("\n*** 0x%08x: %s (vector %d)\n", tf->tf_pc, trapname(vec), vec);
    printf("*** sr %04x, frame format %x\n", tf->tf_sr, tf->tf_format);
    switch (tf->tf_format) {
    case 0x2:
        printf("*** instruction address 0x%08x\n", tf->F_u.F_fmt2.f_iaddr);
        break;
    case 0xA:
    case 0xB:
        printf("*** fault address 0x%08x, ssw %04x\n",
            tf->F_u.F_fmtA.f_dcfa, tf->F_u.F_fmtA.f_ssw);
        break;
    }
    printf("*** registers:\n");
    for (i = 0; i < 7; i++)
        printf("d%d = %08x   a%d = %08x\n",
            i, tf->tf_regs[i], i, tf->tf_regs[8 + i]);
    printf("d7 = %08x  usp = %08x  ssp = %08x\n", tf->tf_regs[7], tf->tf_sp,
        (u_int) &tf->tf_sr + framesize[tf->tf_format]);
}

/*
 * A user bus or address error is not restarted (D26).  Drop the format
 * A or B state so that rte resumes at tf_pc from a format 0 frame;
 * trapret in locore.S moves SR and PC up by tf_stackadj bytes.
 */
static void
trimframe(struct trapframe *tf)
{
    tf->tf_stackadj = framesize[tf->tf_format] - FMT0SIZE;
    tf->tf_format = 0;
    tf->tf_vector = 0;
}

/*
 * Clear the pending exception in the 68881/2 state frame saved by
 * fpfault, before frestore.  Byte 0 is the frame version (0 is the null
 * frame), byte 1 the size of the rest; the frame ends with the BIU flags
 * long word, whose bit 27 is set by the handler to acknowledge the
 * exception.  After NetBSD's fpfault.
 */
void
fpfault_clear(u_char *state)
{
    if (state[0] != 0)
        state[state[1]] |= 0x08;
}

static u_int
getfpsr(void)
{
    u_int fpsr;

    __asm__ volatile ("fmove.l %%fpsr, %0" : "=d" (fpsr));
    return fpsr;
}

/*
 * Record the user stack size for swapping, from the user sp.  Returns 0
 * if the stack has run into the data segment.
 */
static int
growstack(struct trapframe *tf)
{
    u_int sp = tf->tf_sp;

    if (sp < u.u_procp->p_daddr + u.u_dsize)
        return 0;
    if (u.u_procp->p_ssize < USER_DATA_END - sp) {
        u.u_procp->p_ssize = USER_DATA_END - sp;
        u.u_procp->p_saddr = sp;
        u.u_ssize = u.u_procp->p_ssize;
    }
    return 1;
}

/*
 * Interrupt.  The clock and the devices come in M3 task 2; until then
 * every interrupt is stray.
 */
static void
intr(struct trapframe *tf, int vec)
{
    printf("*** stray interrupt, vector %d, level %d\n",
        vec, (tf->tf_sr & PSL_IPL) >> 8);
}

/*
 * Called from locore.S for every exception and interrupt.
 */
void
trap(struct trapframe *tf)
{
    int vec = tf->tf_vector >> 2;
    int psig = 0;
    time_t syst;

    if ((u_int) tf < (u_int) &u + sizeof(u)) {
        dumpregs(tf);
        panic("stack overflow");
        /*NOTREACHED*/
    }

    if (! USERMODE(tf->tf_sr)) {
        if (ISINTR(vec)) {
            intr(tf, vec);
            return;
        }
        if (vec == T_TRACE &&
            (tf->tf_pc == (u_int) trapentry || tf->tf_pc == (u_int) fpfault)) {
            /*
             * A traced user trap instruction: the 68030 takes the trace
             * after the trap, at the first handler instruction.  Report
             * it to the process; the handler then runs as usual.
             */
            psignal(u.u_procp, SIGTRAP);
            return;
        }
        dumpregs(tf);
        panic("kernel trap");
        /*NOTREACHED*/
    }

    u.u_frame = tf;
    syst = u.u_ru.ru_stime;

    if (ISINTR(vec)) {
        intr(tf, vec);
        if (! runrun)
            return;
        u.u_code = tf->tf_pc;           /* for the signal handler */
        spl0();
        if (growstack(tf))
            goto out;
        psig = SIGSEGV;
        goto post;
    }

#ifdef UCB_METER
    cnt.v_trap++;
#endif
    u.u_code = vec;
    switch (vec) {
    case T_BUSERR:
    case T_ADDRERR:
        u.u_code = faultaddr(tf);
        printf("*** 0x%08x: %s: %s, address 0x%08x\n",
            tf->tf_pc, u.u_comm, trapname(vec), u.u_code);
        trimframe(tf);
        psig = SIGBUS;
        break;
    case T_ILLINST:
    case T_PRIVINST:
    case T_LINEA:
    case T_LINEF:
        psig = SIGILL;
        break;
    case T_ZERODIV:
    case T_CHK:
    case T_TRAPV:
    case T_COPERR:
        psig = SIGFPE;
        break;
    case T_TRACE:
        tf->tf_sr &= ~PSL_T;
        psig = SIGTRAP;
        break;
    default:
        if (vec >= T_TRAP0 && vec <= T_TRAP15) {
            psig = SIGILL;              /* trap #0 is M3 task 5 */
            break;
        }
        if (vec >= T_FPFIRST && vec <= T_FPLAST) {
            u.u_code = getfpsr();
            psig = SIGFPE;
            break;
        }
        dumpregs(tf);
        panic("unexpected user trap");
        /*NOTREACHED*/
    }
post:
    psignal(u.u_procp, psig);
out:
    /* Process all received signals. */
    for (;;) {
        psig = CURSIG(u.u_procp);
        if (psig <= 0)
            break;
        postsig(psig);
    }
    curpri = setpri(u.u_procp);

    /* Switch to another process. */
    if (runrun) {
        setrq(u.u_procp);
        u.u_ru.ru_nivcsw++;
        swtch();
    }

    /* Update profiling information. */
    if (u.u_prof.pr_scale)
        addupc((caddr_t) tf->tf_pc, &u.u_prof, (int) (u.u_ru.ru_stime - syst));
}
