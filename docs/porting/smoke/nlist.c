/*
 * sysctl CPU_NLIST from user mode (sys/m68k/sysctl.c, kern_sysctl.c), as
 * an ordinary user (D35): the address of a kernel symbol; unknown names,
 * a bad name pointer and names over 16 bytes are refused.  Writes to
 * other top levels (the host name) still need root.
 */
#include "sysctltest.h"
#include <sys/kernel.h>

static int *addr;
static size_t *len;

void
got_signal(int sig)
{
    check("signal", sig == SIGILL);
    check("result", call_error(0) == 0 && results[0][0] == sizeof(int));
    check("address", *addr == (int) &hz);
    check("length", *len == sizeof(int));
    check("unknown", call_error(1) == EOPNOTSUPP);
    check("bad name", call_error(2) == EFAULT);
    check("long name", call_error(3) == EOPNOTSUPP);
    check("kern root only", call_error(4) == EPERM);
    report("nlist");
}

/* Copy s, with its terminating zero, to the user region. */
static char *
ustr(const char *s)
{
    int n = strlen(s) + 1;
    char *p = ualloc(n);

    bcopy(s, p, n);
    return p;
}

int
main()
{
    static const int mib[] = { CTL_MACHDEP, CPU_NLIST };
    static const int hostname[] = { CTL_KERN, KERN_HOSTNAME };
    size_t *lens;
    int i;

    ttest_init();
    u.u_uid = 100;
    addr = ualloc(sizeof *addr);
    len = ualloc(sizeof *len);
    lens = ualloc(4 * sizeof *lens);
    *addr = 0;
    *len = sizeof *addr;
    for (i = 0; i < 4; i++)
        lens[i] = 64;
    add_call(mib, 2, addr, len, ustr("_hz"), 4);
    add_call(mib, 2, ualloc(64), &lens[0], ustr("_nosuch"), 8);
    add_call(mib, 2, ualloc(64), &lens[1], (void *) (USER_DATA_END + 16), 4);
    add_call(mib, 2, ualloc(64), &lens[2], ustr("_0123456789abcdef"), 18);
    add_call(hostname, 2, ualloc(64), &lens[3], ustr("x"), 2);
    run_calls();
    return 0;
}
