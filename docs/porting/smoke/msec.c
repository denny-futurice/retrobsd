/*
 * msec system call (sys/m68k/clock.c): sc_msec() returns the
 * milliseconds since clkstart(), 1000 / HZ per tick.
 */
#include "ttest.h"
#include <sys/kernel.h>
#include <machine/virt.h>

void sc_msec(void);

void
got_signal(int sig)
{
    check("no signal", 0);
}

static u_int
rtcnow(void)
{
    return VIRT_REG(VIRT_RTC_TICK + GF_RTC_TIME_LOW);
}

int
main()
{
    u_int start;
    int ticks;

    ttest_init();
    noproc = 1;                         /* no process to charge ticks to */
    clkstart();
    start = rtcnow();
    spl0();
    while (rtcnow() - start < 200000000)        /* 200 ms */
        continue;
    splhigh();
    sc_msec();
    ticks = time.tv_sec * hz + lbolt;
    check("ticks", u.u_rval == ticks * (1000 / HZ));
    check("msec", u.u_rval >= 180 && u.u_rval <= 205);
    report("msec");
}
