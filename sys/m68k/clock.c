/*
 * Clock for the 68030 on QEMU virt (clock.c on PIC32).  The Goldfish
 * RTC 1 alarm is the hz tick, RTC 2 holds the time of day (D27).
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/frame.h>
#include <machine/virt.h>

#define TICK_NS         (1000000000 / HZ)
#define RTC(base, reg)  VIRT_REG((base) + (reg))

static unsigned long long nexttick;     /* RTC 1 time of the next tick */

static void
rtc_reset(u_int base)
{
    RTC(base, GF_RTC_IRQ_ENABLED) = 0;
    RTC(base, GF_RTC_CLEAR_ALARM) = 0;
    RTC(base, GF_RTC_CLEAR_INTERRUPT) = 0;
}

static void
setalarm(unsigned long long t)
{
    RTC(VIRT_RTC_TICK, GF_RTC_ALARM_HIGH) = t >> 32;
    RTC(VIRT_RTC_TICK, GF_RTC_ALARM_LOW) = t;
}

/*
 * Setup the RTC 1 alarm for `hz' timer interrupts per second.
 */
void
clkstart()
{
    int s = splhigh();
    u_int lo;

    rtc_reset(VIRT_RTC_TICK);
    rtc_reset(VIRT_RTC_TOD);
    lo = RTC(VIRT_RTC_TICK, GF_RTC_TIME_LOW);
    nexttick = (unsigned long long) RTC(VIRT_RTC_TICK, GF_RTC_TIME_HIGH) << 32 | lo;
    nexttick += TICK_NS;
    RTC(VIRT_RTC_TICK, GF_RTC_IRQ_ENABLED) = 1;
    setalarm(nexttick);
    VIRT_REG(GF_PIC_ADDR(VIRT_RTC_TICK_LEVEL, GF_PIC_ENABLE)) =
        GF_PIC_BIT(VIRT_RTC_TICK_SRC);
    splx(s);
}

/*
 * The tick, from intr() at level 6.  The next alarm is one tick after
 * the previous one, not after now, so ticks do not drift.  If ticks were
 * missed it is already due and fires again as soon as this returns: the
 * missed ticks come in a burst (as in NetBSD).
 */
void
clockintr(struct trapframe *tf)
{
    RTC(VIRT_RTC_TICK, GF_RTC_CLEAR_INTERRUPT) = 0;
    nexttick += TICK_NS;
    setalarm(nexttick);
    hardclock((caddr_t) tf->tf_pc, tf->tf_sr);
}

/*
 * Set the time of day from RTC 2.  base is the root filesystem time,
 * used instead if the RTC reads earlier than that.
 */
void
inittodr(time_t base)
{
    u_int quot, rem;
    int s, ok;

    s = splhigh();
    quot = RTC(VIRT_RTC_TOD, GF_RTC_TIME_LOW);
    rem = RTC(VIRT_RTC_TOD, GF_RTC_TIME_HIGH);

    /*
     * Seconds and nanoseconds by one 64/32 bit divide.  The quotient
     * overflows 32 bits if the high word is 1e9 or more: after 2106,
     * or a negative time.
     */
    ok = rem < 1000000000;
    if (ok)
        __asm__ ("divu.l %2, %0:%1" : "+d" (rem), "+d" (quot) : "d" (1000000000));
    ok = ok && (time_t) quot >= base;
    if (ok) {
        time.tv_sec = quot;
        time.tv_usec = rem / 1000;
    } else {
        time.tv_sec = base;
        time.tv_usec = 0;
    }
    lbolt = time.tv_usec / usechz;
    splx(s);

    if (! ok)
        printf("WARNING: RTC behind file system time -- CHECK AND RESET THE DATE!\n");
}
