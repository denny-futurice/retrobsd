/*
 * Common part of the libc tests (libcrt0 to libsignal): each is a C
 * /sbin/init linked with src/crt0.o and src/libc.a.  It opens the console
 * as fds 0-2, prints "FAIL file:line: condition" for every failed check
 * and "<name> ok" if none failed, and halts with reboot(RB_HALT).
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>

static int failed;

#define check(c) do { \
        if (!(c)) { \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            failed++; \
        } \
    } while (0)

static inline void
console(void)
{
    open("/dev/console", O_RDWR);
    dup(0);
    dup(0);
}

static inline void
finish(const char *name)
{
    if (!failed)
        printf("%s ok\n", name);
    fflush(stdout);
    reboot(RB_HALT);
}
