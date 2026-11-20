/*
 * crt0 (D30): process 1, started by icode as "init -", finds argc, argv
 * and an empty environment.  A child execs this program again with a
 * path, two arguments and two environment strings and returns 7 from
 * main if it sees them, so crt0 must pass main's result to exit.
 */
#include "libtest.h"
#include <sys/wait.h>

extern const char *__progname;

int
main(int argc, char **argv, char **envp)
{
    char *cargv[] = { "/sbin/init", "child", 0 };
    char *cenvp[] = { "A=1", "B=2", 0 };
    int pid, status;

    if (getpid() != 1)
        return (argc == 2 && strcmp(argv[0], "/sbin/init") == 0 &&
                strcmp(argv[1], "child") == 0 && argv[2] == 0 &&
                envp == argv + 3 && environ == envp &&
                strcmp(envp[0], "A=1") == 0 && strcmp(envp[1], "B=2") == 0 &&
                envp[2] == 0 && strcmp(__progname, "init") == 0) ? 7 : 1;

    console();
    check(argc == 2);
    check(strcmp(argv[0], "init") == 0);
    check(strcmp(argv[1], "-") == 0);         /* "-s" in single user */
    check(argv[2] == 0);
    check(envp == argv + 3);
    check(environ == envp);
    check(envp[0] == 0);
    check(strcmp(__progname, "init") == 0);

    pid = fork();
    if (pid == 0) {
        execve(cargv[0], cargv, cenvp);
        _exit(2);
    }
    check(pid > 1);
    check(wait(&status) == pid);
    check(status == 7 << 8);
    finish("libcrt0");
    return 0;
}
