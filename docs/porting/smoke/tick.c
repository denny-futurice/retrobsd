/*
 * Clock tick (sys/m68k/clock.c, trap.c): after clkstart() the RTC 1 alarm
 * interrupts at HZ and hardclock() counts the ticks.
 */
#include "ttest.h"
#include <sys/kernel.h>
#include <sys/vmmeter.h>
#include <machine/virt.h>

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
    while (rtcnow() - start < 200000000)        /* 200 ms: 40 ticks */
        continue;
    splhigh();
    ticks = time.tv_sec * hz + lbolt;
    check("ticks", ticks >= 36 && ticks <= 41);
    check("interrupts", cnt.v_intr == ticks);
    report("tick");
}
