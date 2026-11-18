/*
 * sysctl CPU_CONSDEV from user mode (sys/m68k/sysctl.c, kern_sysctl.c):
 * returns the console device and its size; new data is refused.
 */
#include "sysctltest.h"

static const int mib[] = { CTL_MACHDEP, CPU_CONSDEV };
static dev_t *dev;
static size_t *len;

void
got_signal(int sig)
{
    check("signal", sig == SIGILL);
    check("result", call_error(0) == 0 && results[0][0] == sizeof(dev_t));
    check("device", *dev == makedev(UART_MAJOR, 0));
    check("length", *len == sizeof(dev_t));
    check("new refused", call_error(1) == EPERM);
    report("consdev");
}

int
main()
{
    size_t *len2;

    ttest_init();
    dev = ualloc(sizeof *dev);
    len = ualloc(sizeof *len);
    len2 = ualloc(sizeof *len2);
    *dev = -1;
    *len = sizeof *dev;
    *len2 = sizeof *dev;
    add_call(mib, 2, dev, len, 0, 0);
    add_call(mib, 2, ualloc(4), len2, dev, sizeof *dev);
    run_calls();
    return 0;
}
