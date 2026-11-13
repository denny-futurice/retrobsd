/*
 * Time of day (sys/m68k/clock.c inittodr()): read from RTC 2 (D27), or
 * the file system time if the RTC is behind it.
 */
#include "ttest.h"
#include <sys/kernel.h>
#include <machine/virt.h>

#define SEC     1000000000              /* RTC 2 is set to SEC + 250 ms */

void
got_signal(int sig)
{
    check("no signal", 0);
}

int
main()
{
    unsigned long long ns = SEC * 1000000000ULL + 250000000;

    ttest_init();
    VIRT_REG(VIRT_RTC_TOD + GF_RTC_TIME_HIGH) = ns >> 32;
    VIRT_REG(VIRT_RTC_TOD + GF_RTC_TIME_LOW) = ns;

    /* RTC 1 still runs at the host time, far from SEC. */
    inittodr(SEC - 1000);
    check("rtc sec", time.tv_sec == SEC);
    check("rtc usec", time.tv_usec >= 250000 && time.tv_usec < 260000);
    check("lbolt", lbolt == time.tv_usec / usechz);

    inittodr(SEC + 1000);
    check("base sec", time.tv_sec == SEC + 1000);
    check("base usec", time.tv_usec == 0 && lbolt == 0);
    report("todr");
}
