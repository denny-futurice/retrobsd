/*
 * The system call stubs (D23): the error path through cerror (errno and
 * -1 in d0 and a0), errno left alone on success, the move.l form for
 * numbers above 127 (mkdir, rmdir), and the hand-written stubs: pipe,
 * _brk, ptrace (which clears errno) and fork through the generated stub.
 */
#include "libtest.h"
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>

int
main(void)
{
    void *(*popen)(const char *, int) = (void *) open;
    int fds[2], status, pid;
    char buf[8];
    char *p;

    console();

    errno = 0;
    check(open("/nonexistent", O_RDONLY) == -1);
    check(errno == ENOENT);
    errno = 0;
    check(popen("/nonexistent", O_RDONLY) == (void *) -1);
    check(errno == ENOENT);
    errno = 99;
    check(getpid() == 1);
    check(errno == 99);

    check(mkdir("/d", 0755) == 0);
    check(mkdir("/d", 0755) == -1 && errno == EEXIST);
    check(rmdir("/d") == 0);
    check(rmdir("/d") == -1 && errno == ENOENT);

    fds[0] = fds[1] = -1;
    check(pipe(fds) == 0);
    check(fds[0] == 3 && fds[1] == 4);
    check(write(fds[1], "pipe", 5) == 5);
    check(read(fds[0], buf, sizeof buf) == 5 && strcmp(buf, "pipe") == 0);
    check(close(fds[0]) == 0 && close(fds[1]) == 0);
    check(close(fds[0]) == -1 && errno == EBADF);

    p = sbrk(0);
    check(sbrk(1024) == p);
    check(sbrk(0) == p + 1024);
    memset(p, 0x5a, 1024);
    check(_brk(p + 2048) == (int) (p + 2048));
    errno = 0;
    check(_brk((void *) 0x7ffffff0) == -1 && errno == ENOMEM);

    pid = fork();
    if (pid == 0) {
        errno = 99;
        _exit(ptrace(PT_TRACE_ME, 0, 0, 0) == 0 && errno == 0 ? 3 : 4);
    }
    check(pid > 1);
    check(wait(&status) == pid);
    check(status == 3 << 8);
    errno = 0;
    check(ptrace(PT_READ_D, 9999, 0, 0) == -1 && errno == ESRCH);

    finish("libsys");
    return 0;
}
