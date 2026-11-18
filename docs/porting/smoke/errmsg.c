/*
 * sysctl CPU_ERRMSG from user mode (sys/m68k/sysctl.c, kern_sysctl.c):
 * the message for an errno and its length; errno 0 and numbers past the
 * table are refused, and so is a buffer too short for the message.
 */
#include "sysctltest.h"

#define MSG "Invalid argument"

static char *buf;
static size_t *len;

static int
same(const char *a, const char *b)
{
    while (*a && *a == *b)
        a++, b++;
    return *a == *b;
}

void
got_signal(int sig)
{
    check("signal", sig == SIGILL);
    check("result", call_error(0) == 0 && results[0][0] == sizeof MSG);
    check("message", same(buf, MSG));
    check("length", *len == sizeof MSG);
    check("errno 0", call_error(1) == EOPNOTSUPP);
    check("past table", call_error(2) == EOPNOTSUPP);
    check("short buffer", call_error(3) == ENOMEM);
    report("errmsg");
}

int
main()
{
    static const int einval[] = { CTL_MACHDEP, CPU_ERRMSG, EINVAL };
    static const int zero[] = { CTL_MACHDEP, CPU_ERRMSG, 0 };
    static const int past[] = { CTL_MACHDEP, CPU_ERRMSG, EAUTH + 2 };
    size_t *len1, *len2, *shortlen;

    ttest_init();
    buf = ualloc(64);
    len = ualloc(sizeof *len);
    len1 = ualloc(sizeof *len1);
    len2 = ualloc(sizeof *len2);
    shortlen = ualloc(sizeof *shortlen);
    *len = *len1 = *len2 = 64;
    *shortlen = sizeof MSG - 1;
    add_call(einval, 3, buf, len, 0, 0);
    add_call(zero, 3, buf, len1, 0, 0);
    add_call(past, 3, buf, len2, 0, 0);
    add_call(einval, 3, ualloc(64), shortlen, 0, 0);
    run_calls();
    return 0;
}
