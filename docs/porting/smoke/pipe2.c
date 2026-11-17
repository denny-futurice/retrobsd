/*
 * pipe() from user mode (sys/kernel/sys_pipe.c, trap.c, D23): the read
 * descriptor comes back in d0 and the write descriptor in d1.
 */
#include "ttest.h"
#include <sys/file.h>
#include <sys/inode.h>

/* sys_pipe.c's callees: two descriptors, 3 and 4, on one inode. */
int selwait;
static struct inode pino;
static struct file files[2];
static int nfile;

struct file *
falloc()
{
    u.u_rval = 3 + nfile;
    return &files[nfile++];
}

struct inode *ialloc(struct inode *pip) { return &pino; }
void iput(struct inode *ip) { }
void ilock(struct inode *ip) { }
int ino_ioctl(struct file *fp, u_int com, caddr_t data) { return 0; }
int rwip(struct inode *ip, struct uio *uio, int ioflag) { return 0; }
void selwakeup(struct proc *p, long coll) { }
void sleep(caddr_t chan, int pri) { }
void wakeup(caddr_t chan) { }

const struct sysent sysent[] = { { 0, nosys }, { 0, pipe } };
const int nsysent = 2;

int fds[2];
short ccr;

extern char pipe2_code[];
__asm__ (
"       .text\n"
"       .globl  pipe2_code\n"
"pipe2_code:\n"
"       move.l  usertop, %sp\n"
"       moveq   #1, %d0\n"
"       moveq   #-1, %d1\n"
"       move.w  #1, %ccr\n"
"       trap    #0\n"
"       move.w  %ccr, ccr\n"
"       movem.l %d0-%d1, fds\n"
"       illegal\n");

void
got_signal(int sig)
{
    check("signal", sig == SIGILL);
    check("d0", fds[0] == 3);
    check("d1", fds[1] == 4);
    check("carry", ! (ccr & PSL_C));
    check("files", files[0].f_flag == FREAD && files[1].f_flag == FWRITE);
    report("pipe2");
}

int
main()
{
    ttest_init();
    enter_user((void (*)(void)) pipe2_code, 0);
    return 0;
}
