/*
 * timeout() (sys/kernel/kern_clock.c) driven by the clock tick
 * (sys/m68k/clock.c): the callback runs on the tick it was due.
 */
#include "ttest.h"
#include <sys/kernel.h>
#include <machine/virt.h>

static int arg, firedtick;

void
got_signal(int sig)
{
    check("no signal", 0);
}

static void
callback(caddr_t a)
{
    arg = (int) a;
    firedtick = time.tv_sec * hz + lbolt;
}

int
main()
{
    u_int start;

    ttest_init();
    noproc = 1;
    coutinit();
    clkstart();
    timeout(callback, (caddr_t) 42, 10);
    start = VIRT_REG(VIRT_RTC_TICK + GF_RTC_TIME_LOW);
    spl0();
    while (! arg && VIRT_REG(VIRT_RTC_TICK + GF_RTC_TIME_LOW) - start < 200000000)
        continue;
    splhigh();
    check("fired", arg == 42);
    check("tick", firedtick == 10);
    report("callout");
}
