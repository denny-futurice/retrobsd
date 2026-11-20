/*
 * <sys/wait.h> (D37): a child's exit code, a terminating signal with and
 * without a core dump, and a stop, as the parent sees them through the
 * W* macros and through the union wait fields (sh, csh, system, popen);
 * and union wait fields set by hand land at the bits of w_status that
 * W_EXITCODE and the kernel use.
 */
#include "libtest.h"
#include <signal.h>
#include <sys/wait.h>

static int
child(int how)
{
    int pid = fork(), st = -1;

    if (pid == 0) {
        switch (how) {
        case 0:
            _exit(42);
        case 1:
            kill(getpid(), SIGTERM);
            break;
        case 2:
            kill(getpid(), SIGQUIT);            /* dumps core as root */
            break;
        case 3:
            kill(getpid(), SIGSTOP);
            _exit(5);
        }
        _exit(99);
    }
    check(pid > 0);
    check(waitpid(pid, &st, WUNTRACED) == pid);
    if (how == 3 && WIFSTOPPED(st)) {
        int st2 = -1;

        kill(pid, SIGCONT);
        check(waitpid(pid, &st2, 0) == pid);
        check(WIFEXITED(st2) && WEXITSTATUS(st2) == 5);
    }
    return st;
}

int
main(void)
{
    union wait w;
    int st;

    console();

    st = child(0);
    check(st == W_EXITCODE(42, 0));
    check(WIFEXITED(st) && !WIFSIGNALED(st) && !WIFSTOPPED(st));
    check(WEXITSTATUS(st) == 42);
    w.w_status = st;
    check(w.w_retcode == 42 && w.w_termsig == 0 && w.w_stopval != WSTOPPED);
    check(WIFEXITED(w) && WEXITSTATUS(w) == 42);

    st = child(1);
    check(st == SIGTERM);
    check(WIFSIGNALED(st) && !WIFEXITED(st) && !WIFSTOPPED(st));
    check(WTERMSIG(st) == SIGTERM && !WCOREDUMP(st));
    w.w_status = st;
    check(w.w_termsig == SIGTERM && w.w_coredump == 0 && w.w_retcode == 0);

    st = child(2);
    check(st == (SIGQUIT | 0200));
    check(WIFSIGNALED(st) && WTERMSIG(st) == SIGQUIT && WCOREDUMP(st) == 1);
    w.w_status = st;
    check(w.w_termsig == SIGQUIT && w.w_coredump == 1);
    check(unlink("/init.core") == 0);

    st = child(3);
    check(st == W_STOPCODE(SIGSTOP));
    check(WIFSTOPPED(st) && !WIFEXITED(st) && !WIFSIGNALED(st));
    check(WSTOPSIG(st) == SIGSTOP);
    w.w_status = st;
    check(w.w_stopval == WSTOPPED && w.w_stopsig == SIGSTOP);

    w.w_status = 0;                             /* as system() does */
    w.w_retcode = 127;
    check(w.w_status == W_EXITCODE(127, 0));
    w.w_status = 0;
    w.w_termsig = SIGKILL;
    w.w_coredump = 1;
    check(w.w_status == (SIGKILL | 0200));
    w.w_status = 0;
    w.w_stopval = WSTOPPED;
    w.w_stopsig = SIGTSTP;
    check(w.w_status == W_STOPCODE(SIGTSTP));

    finish("libwait");
    return 0;
}
