/*
 * Support for the sysctl smoke tests, which link sys/kernel/kern_sysctl.c
 * and sys/m68k/sysctl.c: placeholders for the kernel variables and
 * functions they refer to, and user-mode code that makes a list of
 * __sysctl() calls through trap #0.  Arguments and buffers live in the
 * user region, where copyin and copyout accept them.
 */
#include "ttest.h"
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>

/*
 * Kernel variables and functions the two files refer to but the tests do
 * not use, defined in assembly so their C types do not matter.  The
 * functions panic.
 */
__asm__ (
"       .text\n"
"       .globl  biowait, brelse, geteblk, sleep, wakeup\n"
"biowait:\n"
"brelse:\n"
"geteblk:\n"
"sleep:\n"
"wakeup:\n"
"       pea     unused\n"
"       jsr     panic\n"
"       .section .rodata\n"
"unused: .asciz  \"sysctl test: unused stub called\"\n"
"       .comm   allproc, 4\n"
"       .comm   avenrun, 4\n"
"       .comm   swapdev, 4\n"
"       .comm   version, 4\n"
"       .comm   bdevsw, 4\n"
"       .comm   boottime, 8\n"
"       .comm   cdevsw, 4\n"
"       .comm   cnttys, 4\n"
"       .comm   forkstat, 4\n"
"       .comm   freemem, 4\n"
"       .comm   ipc, 4\n"
"       .comm   nblkdev, 4\n"
"       .comm   nchrdev, 4\n"
"       .comm   nchstats, 4\n"
"       .comm   nswap, 4\n"
"       .comm   rate, 4\n"
"       .comm   runin, 4\n"
"       .comm   runout, 4\n"
"       .comm   selwait, 4\n"
"       .comm   sum, 4\n"
"       .comm   tk_nin, 4\n"
"       .comm   tk_nout, 4\n"
"       .comm   total, 4\n"
"       .comm   zombproc, 4\n");

/* As in ufs_fio.c. */
int
suser()
{
    if (u.u_uid == 0)
        return 1;
    u.u_error = EPERM;
    return 0;
}

/* One call: sp points at pad, so the arguments start at usp + 4 (D23). */
struct ucall {
    int     pad;
    int     *name;
    u_int   namelen;
    void    *old;
    size_t  *oldlenp;
    void    *new;
    size_t  newlen;
};

#define MAXCALLS 8

struct ucall *ucalls;                   /* in the user region */
int nucalls;
int results[MAXCALLS][2];               /* d0, ccr after each call */

const struct sysent sysent[] = { { 0, nosys }, { 6, __sysctl } };
const int nsysent = 2;

extern char sysctl_code[];
__asm__ (
"       .text\n"
"       .globl  sysctl_code\n"
"sysctl_code:\n"
"       movea.l ucalls, %a2\n"
"       lea     results, %a3\n"
"       move.l  nucalls, %d3\n"
"       bra.s   2f\n"
"1:     move.l  %a2, %sp\n"
"       moveq   #1, %d0\n"
"       trap    #0\n"
"       move.w  %ccr, %d1\n"
"       move.l  %d0, (%a3)+\n"
"       move.l  %d1, (%a3)+\n"
"       lea     28(%a2), %a2\n"
"2:     subq.l  #1, %d3\n"
"       bge.s   1b\n"
"       illegal\n");

static char *ubrk = (char *) USER_DATA_START;

/* Allocate n bytes, long aligned, in the user region. */
static void *
ualloc(int n)
{
    void *p = ubrk;

    ubrk += (n + 3) & ~3;
    return p;
}

/* Add a call to the list; name is copied to the user region. */
static struct ucall *
add_call(const int *name, int namelen, void *old, size_t *oldlenp,
    void *new, size_t newlen)
{
    struct ucall *c;
    int *uname = ualloc(namelen * sizeof(int));

    bcopy(name, uname, namelen * sizeof(int));
    if (nucalls == 0)
        ucalls = ualloc(MAXCALLS * sizeof(struct ucall));
    c = &ucalls[nucalls++];
    c->name = uname;
    c->namelen = namelen;
    c->old = old;
    c->oldlenp = oldlenp;
    c->new = new;
    c->newlen = newlen;
    return c;
}

/* Result of call i: 0 on success, else the errno. */
static int
call_error(int i)
{
    return (results[i][1] & PSL_C) ? results[i][0] : 0;
}

/* Run the calls in user mode; got_signal() gets SIGILL at the end. */
static void
run_calls()
{
    enter_user((void (*)(void)) sysctl_code, 0);
}
