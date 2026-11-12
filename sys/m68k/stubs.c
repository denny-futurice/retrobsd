/*
 * Placeholders for kernel symbols that M3 implements (decision D20), so the
 * M2 kernel links.  Every function panics with "<name> not implemented";
 * none is reached on the way to the M2 finish line, the "No root
 * filesystem found!" panic.  M3 removes each entry as the real code
 * arrives, and then this file.
 *
 * The trap-frame files kernel/exec_subr.c, sys_pipe.c and sys_process.c
 * are left out of files.kconf until the m68k trap frame exists (M3); the
 * symbols they define are stubbed here.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/exec.h>
#include <sys/signal.h>
#include <sys/sysctl.h>

#define NOTYET(name)    panic(#name " not implemented")

/*
 * Context switch: startup.S on PIC32.
 */
int
setjmp(label_t *env)
{
    NOTYET(setjmp);
    return 0;
}

void
longjmp(size_t unew, label_t *env)
{
    NOTYET(longjmp);
}

/*
 * User code for process 1, which execs /sbin/init: startup.S on PIC32.
 * Empty here; process 1 is not started without a root filesystem.
 */
__asm__ (
"       .section .rodata\n"
"       .globl  icode, initflags, icodeend\n"
"icode:\n"
"initflags:\n"
"       .byte   0\n"
"icodeend:\n"
"       .previous\n");

/*
 * Signals: signal.c on PIC32.
 */
void
sendsig(sig_t p, int sig, long mask)
{
    NOTYET(sendsig);
}

void
sigreturn()
{
    NOTYET(sigreturn);
}

/*
 * System calls: exception.c and sysctl.c on PIC32.
 */
void
nosys()
{
    NOTYET(nosys);
}

void
sc_msec()
{
    NOTYET(sc_msec);
}

void
ucall()
{
    NOTYET(ucall);
}

void
ufetch()
{
    NOTYET(ufetch);
}

void
ustore()
{
    NOTYET(ustore);
}

int
cpu_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen)
{
    NOTYET(cpu_sysctl);
    return 0;
}

/*
 * kernel/exec_subr.c: sets up the user registers in the trap frame.
 */
void
exec_setupstack(unsigned entryaddr, struct exec_params *epp)
{
    NOTYET(exec_setupstack);
}

void
exec_alloc_freeall(struct exec_params *epp)
{
    NOTYET(exec_alloc_freeall);
}

void *
exec_alloc(int size, int ru, struct exec_params *epp)
{
    NOTYET(exec_alloc);
    return 0;
}

int
exec_estab(struct exec_params *epp)
{
    NOTYET(exec_estab);
    return 0;
}

void
exec_save_args(struct exec_params *epp)
{
    NOTYET(exec_save_args);
}

void
exec_clear(struct exec_params *epp)
{
    NOTYET(exec_clear);
}

/*
 * kernel/sys_pipe.c: pipe() returns its second descriptor in a register.
 */
void
pipe()
{
    NOTYET(pipe);
}

static int
pipe_rw(struct file *fp, struct uio *uio)
{
    NOTYET(pipe_rw);
    return 0;
}

static int
pipe_ioctl(struct file *fp, u_int com, char *data)
{
    NOTYET(pipe_ioctl);
    return 0;
}

static int
pipe_select(struct file *fp, int flag)
{
    NOTYET(pipe_select);
    return 0;
}

static int
pipe_close(struct file *fp)
{
    NOTYET(pipe_close);
    return 0;
}

const struct fileops pipeops = {
    pipe_rw, pipe_ioctl, pipe_select, pipe_close
};

/*
 * kernel/sys_process.c: ptrace reads and writes the trap frame.
 */
void
ptrace()
{
    NOTYET(ptrace);
}

int
procxmt()
{
    NOTYET(procxmt);
    return 0;
}
