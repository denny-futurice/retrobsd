/*
 * machdep.c udelay(): busy-waits on the Goldfish RTC.  Measure a 20 ms
 * and a 2.5 s delay with the RTC itself; QEMU may run late but never
 * early.  2.5e9 ns is past the 2^31 limit of udelay's signed compare,
 * so the long delay only works if udelay splits it into steps.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/virt.h>
#include "ktest.h"

static unsigned
elapsed_ns(unsigned usec)
{
    unsigned t0 = VIRT_REG(VIRT_GF_RTC_BASE + GF_RTC_TIME_LOW);

    udelay(usec);
    return VIRT_REG(VIRT_GF_RTC_BASE + GF_RTC_TIME_LOW) - t0;
}

int main(void)
{
    unsigned ns;

    ns = elapsed_ns(20000);
    check("udelay 20 ms", ns >= 20000000 && ns < 200000000);
    ns = elapsed_ns(2500000);
    check("udelay 2.5 s", ns >= 2500000000u && ns < 3500000000u);
    report("udelay");
    return 0;
}
