/*
 * Process 1's user code (icode in sys/m68k/startup.S), run from
 * USER_DATA_START as main() leaves it: execv("/sbin/init", {"init", "-s",
 * 0}) with the single user flag patched in, and exit(errno) when exec
 * fails.  Stub execv and exit system calls check the arguments.
 */
#include "ttest.h"

#define SYS_exit        1               /* include/syscall.h */
#define SYS_execv       11

static int execd;

static int
streq(const char *a, const char *b)
{
    while (*a == *b++)
        if (*a++ == 0)
            return 1;
    return 0;
}

static void
fake_execv()
{
    char *path = (char *) u.u_arg[0];
    char **argv = (char **) u.u_arg[1];

    execd = 1;
    check("path", streq(path, "/sbin/init"));
    check("argv", streq(argv[0], "init") &&
        streq(argv[1], "-s") && argv[2] == 0);
    check("argv on stack", (u_int) argv > USER_DATA_END - 64 &&
        (u_int) argv < USER_DATA_END);
    u.u_error = ENOENT;
}

static void
fake_exit()
{
    check("exec", execd);
    check("status", u.u_arg[0] == ENOENT);
    report("icode");
}

const struct sysent sysent[] = {
    [0 ... SYS_execv] = { 0, nosys },
    [SYS_exit] = { 1, fake_exit },
    [SYS_execv] = { 2, fake_execv },
};
const int nsysent = SYS_execv + 1;

void
got_signal(int sig)
{
    printf("icode: signal %d\n", sig);
}

/* As init_main.c does for RB_SINGLE. */
int
main()
{
    ttest_init();
    bcopy(icode, (caddr_t) USER_DATA_START, icodeend - icode);
    ((char *) USER_DATA_START)[initflags - icode + 1] = 's';
    return 0;
}
