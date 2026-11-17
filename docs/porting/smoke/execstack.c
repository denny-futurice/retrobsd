/*
 * Exec entry stack (sys/kernel/exec_subr.c exec_setupstack(), D30): sp
 * at argc, the argv and envp vectors packed above it, then the strings;
 * the top word points at argv for ps.  The stack fits stack.len exactly.
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
streq(const char *a, const char *b)
{
    while (*a && *a == *b)
        a++, b++;
    return *a == *b;
}

static char *args[] = { "prog", "-x" };
static char *envs[] = { "HOME=/" };
static struct exec_params ep;
static struct trapframe tf;

int
main()
{
    int *sp;
    char **argv, **envp;

    ttest_init();
    u.u_frame = &tf;
    ep.argp = args;
    ep.argc = 2;
    ep.argbc = 5 + 3;
    ep.envp = envs;
    ep.envc = 1;
    ep.envbc = 7;
    /* As exec_aout.c sizes it. */
    ep.stack.len = SSIZE + roundup(ep.argbc + ep.envbc, NBPW) +
        (ep.argc + ep.envc + 4) * NBPW;
    ep.stack.vaddr = (caddr_t) USER_DATA_END - ep.stack.len;
    exec_setupstack(USER_DATA_START + 0x100, &ep);

    sp = (int *) tf.tf_regs[15];
    argv = (char **) (sp + 1);
    envp = argv + ep.argc + 1;
    check("argc", sp[0] == 2);
    check("argv", streq(argv[0], "prog") && streq(argv[1], "-x") && ! argv[2]);
    check("envp", streq(envp[0], "HOME=/") && ! envp[1]);
    check("strings", argv[0] == (char *) (envp + 2));
    check("ps word", *(char ***) (USER_DATA_END - NBPW) == argv);
    check("stack.len", (caddr_t) sp == ep.stack.vaddr + SSIZE);
    check("pc", tf.tf_pc == USER_DATA_START + 0x100);
    check("comm", streq(u.u_comm, "prog"));
    report("execstack");
}
