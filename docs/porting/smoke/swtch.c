/*
 * locore.S setjmp/longjmp: a switch to the process in u0 and back, set up
 * as fork() does it (a copy of u in u0).  The u areas, p_addr, the stack,
 * the return value and the interrupt level are checked on both sides.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include "ktest.h"

static struct proc pa, pb;
static int inb;

static int
ipl(void)
{
    u_short sr;

    asm volatile ("move.w %%sr, %0" : "=d" (sr));
    return (sr >> 8) & 7;
}

int
main(void)
{
    volatile int local = 1;

    u.u_procp = &pa;
    pa.p_addr = (size_t) &u;
    if (setjmp(&u.u_ssave)) {
        /* Process B, running in u. */
        inb = 1;
        check("b ipl", ipl() == 0);
        check("b procp", u.u_procp == &pb);
        check("b p_addr", pb.p_addr == (size_t) &u && pa.p_addr == (size_t) &u0);
        check("b other u", u0.u_procp == &pa && u0.u_error == 0);
        check("b local", local == 2);
        u.u_error = 7;
        local = 3;
        asm volatile ("move.w #0x2700, %sr");
        longjmp(pa.p_addr, &u.u_rsave);
        check("b longjmp returned", 0);
    }
    local = 2;
    u.u_procp = &pb;
    bcopy(&u, &u0, USIZE);
    u.u_procp = &pa;
    pb.p_addr = (size_t) &u0;
    local = 4;
    if (setjmp(&u.u_rsave) == 0) {
        asm volatile ("move.w #0x2700, %sr");
        longjmp(pb.p_addr, &u.u_ssave);
        check("a longjmp returned", 0);
    }
    check("ran b", inb);
    check("a ipl", ipl() == 0);
    check("a procp", u.u_procp == &pa);
    check("a p_addr", pa.p_addr == (size_t) &u && pb.p_addr == (size_t) &u0);
    check("a other u", u0.u_procp == &pb && u0.u_error == 7);
    check("a u", u.u_error == 0);
    check("a local", local == 4);
    report("swtch");
}
