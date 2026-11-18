/*
 * Interrupt handlers do not nest (sys/m68k/trap.c intr(), D33): a clock
 * tick (level 6) that falls due during the console receive interrupt
 * (level 1) waits until that handler returns.
 */
#include "ttest.h"
#include <sys/kernel.h>
#include "ttystub.h"

#define TICKS() (time.tv_sec * hz + lbolt)

static int during;

void
got_signal(int sig)
{
    check("no signal", 0);
}

/*
 * After the first byte, stay in the handler for 4 tick periods.  The
 * device interrupt is off meanwhile: on QEMU a raised but masked
 * interrupt keeps the timers from running (D33), so the tick could not
 * fall due.
 */
static void
first_byte(void)
{
    u_int start = rtcnow();
    int t = TICKS();

    VIRT_REG(VIRT_GF_TTY_BASE + GF_TTY_CMD) = GF_TTY_CMD_INT_DISABLE;
    while (rtcnow() - start < 4 * (1000000000 / HZ))
        continue;
    during = TICKS() - t;
    VIRT_REG(VIRT_GF_TTY_BASE + GF_TTY_CMD) = GF_TTY_CMD_INT_ENABLE;
}

int
main()
{
    u_int start;
    int t;

    ttest_init();
    first_input = first_byte;
    noproc = 1;
    clkstart();
    open_console();
    start = rtcnow();
    spl0();
    while (nin == 0 && rtcnow() - start < 1000000000)
        continue;
    splhigh();
    t = TICKS();
    spl0();
    start = rtcnow();
    while (rtcnow() - start < 20000000)         /* 20 ms */
        continue;
    splhigh();
    check("input", nin > 0);
    check("not nested", during == 0);
    check("ticks resume", TICKS() > t);
    report("intrnest");
}
