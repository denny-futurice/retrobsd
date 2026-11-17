/*
 * locore.S: the FPU registers and FPCR follow each process across a
 * longjmp() switch (D28), and savefpu() copies them into u as fork()
 * needs.  The outgoing state is left in u0.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include "ktest.h"

static struct proc pa, pb;
static int inb;

/* fpN = base + N, and the FPCR rounding mode. */
static void
setfp(int base, u_int fpcr)
{
    asm volatile (
        "fmove.l %0, %%fp0\n"
        "addq.l #1, %0\n fmove.l %0, %%fp1\n"
        "addq.l #1, %0\n fmove.l %0, %%fp2\n"
        "addq.l #1, %0\n fmove.l %0, %%fp3\n"
        "addq.l #1, %0\n fmove.l %0, %%fp4\n"
        "addq.l #1, %0\n fmove.l %0, %%fp5\n"
        "addq.l #1, %0\n fmove.l %0, %%fp6\n"
        "addq.l #1, %0\n fmove.l %0, %%fp7\n"
        "fmove.l %1, %%fpcr"
        : "+d" (base) : "d" (fpcr));
}

/* Do the FPU registers and FPCR hold what setfp(base, fpcr) put there? */
static int
isfp(int base, u_int fpcr)
{
    int r[8], i;
    u_int cr;

    asm volatile (
        "fmove.l %%fp0, (%1)\n fmove.l %%fp1, 4(%1)\n"
        "fmove.l %%fp2, 8(%1)\n fmove.l %%fp3, 12(%1)\n"
        "fmove.l %%fp4, 16(%1)\n fmove.l %%fp5, 20(%1)\n"
        "fmove.l %%fp6, 24(%1)\n fmove.l %%fp7, 28(%1)\n"
        "fmove.l %%fpcr, %0"
        : "=d" (cr) : "a" (r) : "memory");
    for (i = 0; i < 8; i++)
        if (r[i] != base + i)
            return 0;
    return cr == fpcr;
}

/* Does a saved fpstate hold fp7 = base + 7 and the FPCR? */
static int
savedfp(struct fpstate *s, int base, u_int fpcr)
{
    int fp7;

    asm volatile ("fmove.x (%1), %%fp0\n fmove.l %%fp0, %0"
        : "=d" (fp7) : "a" (&s->fps_regs[7 * 3]) : "fp0");
    return fp7 == base + 7 && s->fps_fpcr == fpcr;
}

int
main(void)
{
    u.u_procp = &pa;
    pa.p_addr = (size_t) &u;
    setfp(10, 0x10);
    if (setjmp(&u.u_ssave)) {
        /* Process B: the FPU state A had when it forked. */
        inb = 1;
        check("b fp", isfp(10, 0x10));
        check("a fp saved in u0", savedfp(&u0.u_fpstate, 20, 0x20));
        setfp(30, 0x30);
        longjmp(pa.p_addr, &u.u_rsave);
    }
    savefpu();
    check("savefpu keeps fp", isfp(10, 0x10));
    check("savefpu", savedfp(&u.u_fpstate, 10, 0x10));
    u.u_procp = &pb;
    bcopy(&u, &u0, USIZE);
    u.u_procp = &pa;
    pb.p_addr = (size_t) &u0;
    setfp(20, 0x20);
    if (setjmp(&u.u_rsave) == 0)
        longjmp(pb.p_addr, &u.u_ssave);
    check("ran b", inb);
    check("a fp", isfp(20, 0x20));
    check("b fp saved in u0", savedfp(&u0.u_fpstate, 30, 0x30));
    report("fpswtch");
}
