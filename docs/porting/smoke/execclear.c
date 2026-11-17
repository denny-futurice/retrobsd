/*
 * Exec register reset (sys/kernel/exec_subr.c exec_clear(), D30): the
 * user registers and SR are zeroed, the FPU and u_fpstate reset, and
 * the bss cleared.
 */
#include "ttest.h"
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/buf.h>

/* The rest of exec_subr.c's callees; not reached here. */
struct buf *geteblk() { return 0; }
void brelse(struct buf *bp) { }
int closef(struct file *fp) { return 0; }
void endvfork() { }
void execsigs(struct proc *p) { }

void
got_signal(int sig)
{
    check("no signal", 0);
}

static int
allzero(const void *p, int n)
{
    const char *c = p;

    while (n-- > 0)
        if (*c++)
            return 0;
    return 1;
}

static void
fill(void *p, int c, int n)
{
    char *d = p;

    while (n-- > 0)
        *d++ = c;
}

static struct exec_params ep;
static struct trapframe tf;
static char fpregs[96];
static double one = 1.5;

int
main()
{
    u_int fpcr, fpsr;
    int i;

    ttest_init();
    u.u_frame = &tf;
    u.u_lastfile = -1;
    for (i = 0; i < 16; i++)
        tf.tf_regs[i] = 0x01010101 * (i + 1);
    tf.tf_sr = PSL_USERSR;
    fill(&u.u_fpstate, 0x5a, sizeof(u.u_fpstate));
    __asm__ volatile (
        "fmove.d %0, %%fp0\n"
        "fmove.x %%fp0, %%fp7\n"
        "fmove.l #0x30, %%fpcr\n"       /* round toward +infinity */
        : : "m" (one));
    ep.bss.vaddr = (caddr_t) USER_DATA_START;
    ep.bss.len = 64;
    fill(ep.bss.vaddr, 0xa5, ep.bss.len);
    ep.stack.len = 64;
    ep.stack.vaddr = (caddr_t) USER_DATA_END - ep.stack.len;
    exec_clear(&ep);

    __asm__ volatile (
        "fmovem.x %%fp0-%%fp7, %0\n"
        "fmove.l %%fpcr, %1\n"
        "fmove.l %%fpsr, %2"
        : "=m" (fpregs), "=d" (fpcr), "=d" (fpsr));
    check("registers", allzero(tf.tf_regs, sizeof(tf.tf_regs)));
    check("sr", tf.tf_sr == 0);
    check("fp registers", allzero(fpregs, sizeof(fpregs)));
    check("fpcr", fpcr == 0 && fpsr == 0);
    check("u_fpstate", allzero(&u.u_fpstate, sizeof(u.u_fpstate)));
    check("bss", allzero(ep.bss.vaddr, ep.bss.len));
    report("execclear");
}
