/*
 * ptrace in the traced child (sys/kernel/sys_process.c procxmt(), D22):
 * PT_WRITE_U reaches only the trap frame registers, SR keeps user mode,
 * PT_READ_U reads the 2-byte aligned PC, PT_STEP sets the trace bit.
 */
#include "ttest.h"
#include <sys/ptrace.h>

/* sys_process.c's callees; ptrace() itself is not called. */
struct proc *pfind(int pid) { return 0; }
void setrun(struct proc *p) { }
void sleep(caddr_t chan, int pri) { }
void wakeup(caddr_t chan) { }

void
exit(int rv)
{
    check("exit", 0);
}

void
got_signal(int sig)
{
    check("no signal", 0);
}

#define UOFF(p) ((int) ((char *) (p) - (char *) &u))

/* Run request r with address addr; returns procxmt() or -1 on error. */
static int
req(int r, int addr, int data)
{
    int ret;

    ipc.ip_lock = u.u_procp->p_pid;
    ipc.ip_req = r;
    ipc.ip_addr = (int *) addr;
    ipc.ip_data = data;
    ret = procxmt();
    return ipc.ip_req < 0 ? -1 : ret;
}

int
main()
{
    struct trapframe tf;                /* on the kernel stack in u, as in a trap */
    struct proc *procp;

    ttest_init();
    bzero(&tf, sizeof(tf));
    u.u_frame = &tf;
    procp = u.u_procp;

    check("write d3", req(PT_WRITE_U, UOFF(&tf.tf_regs[3]), 0x1234) == 0 &&
        tf.tf_regs[3] == 0x1234);
    check("write sp", req(PT_WRITE_U, UOFF(&tf.tf_regs[15]), 0x8f000) == 0 &&
        tf.tf_regs[15] == 0x8f000);
    check("write sr", req(PT_WRITE_U, UOFF(&tf.tf_sr), 0x2700 | PSL_USERSR) == 0 &&
        tf.tf_sr == PSL_USERSR);
    check("write pc", req(PT_WRITE_U, UOFF(&tf.tf_pc), 0x80200) == 0 &&
        tf.tf_pc == 0x80200);
    check("read pc", req(PT_READ_U, UOFF(&tf.tf_pc), 0) == 0 && ipc.ip_data == 0x80200);
    check("write u", req(PT_WRITE_U, UOFF(&u.u_procp), 0) == -1 && u.u_procp == procp);
    check("read past u", req(PT_READ_U, USIZE - 2, 0) == -1);

    tf.tf_sr = 0;
    check("continue", req(PT_CONTINUE, 1, 0) == 1 &&
        tf.tf_pc == 0x80200 && tf.tf_sr == 0);
    check("step", req(PT_STEP, 0x80300, 0) == 1 &&
        tf.tf_pc == 0x80300 && (tf.tf_sr & PSL_T));
    report("ptrace");
}
