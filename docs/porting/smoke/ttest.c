/*
 * Support for the smoke tests that link the kernel's locore.S and trap.c
 * (with machdep.c and ktest.c): the scheduler and signal symbols trap()
 * calls, entry to user mode, and hand-built exception frames for what
 * QEMU does not raise (D25).  Not a test itself.
 */
#include "ttest.h"
#include <sys/vmmeter.h>

int runrun;
char curpri;
int ustack[USTACK];
u_int fake_ssp;
struct vmrate cnt;
static struct proc proc0;

void
psignal(struct proc *p, int sig)
{
    got_signal(sig);
}

int
issignal(struct proc *p)
{
    return 0;
}

void
postsig(int sig)
{
}

int
setpri(struct proc *p)
{
    return 0;
}

void
setrq(struct proc *p)
{
}

void
swtch()
{
}

void
ttest_init()
{
    bzero(&u, sizeof(u));
    u.u_procp = &proc0;
}

/* rte to pc in user mode, on the user stack ustack. */
void
enter_user(void (*pc)(void), int sr)
{
    __asm__ volatile (
        "move.l %0, %%usp\n"
        "clr.w  -(%%sp)\n"              /* format 0 */
        "move.l %1, -(%%sp)\n"
        "move.w %2, -(%%sp)\n"
        "rte"
        : : "a" (ustack + USTACK), "a" (pc), "d" (sr) : "memory");
    for (;;)
        continue;
}

/*
 * Push the hardware frame f of size bytes on the kernel stack and enter
 * trapentry, as if the CPU had taken the exception; the user sp is ustack.
 */
void
fake_trap(const struct hwframe *f, int size)
{
    __asm__ volatile (
        "move.l %2, %%usp\n"
        "move.l %%sp, fake_ssp\n"
        "suba.l %1, %%sp\n"
        "move.l %%sp, %%a1\n"
        "1:     move.w (%0)+, (%%a1)+\n"
        "subq.l #2, %1\n"
        "bne.s  1b\n"
        "jmp    trapentry"
        : "+a" (f), "+d" (size) : "a" (ustack + USTACK) : "a1", "memory");
    for (;;)
        continue;
}
