/*
 * The console receive interrupt is off until the first open
 * (sys/m68k/gftty.c): input that is waiting is not delivered before,
 * and arrives at once after.
 */
#include "ttest.h"
#include <sys/kernel.h>
#include "ttystub.h"

void
got_signal(int sig)
{
    check("no signal", 0);
}

int
main()
{
    u_int start;

    ttest_init();
    noproc = 1;
    clkstart();                         /* see ttyrx.c */
    gfttyinit();
    start = rtcnow();
    while (VIRT_REG(VIRT_GF_TTY_BASE + GF_TTY_BYTES_READY) == 0 &&
        rtcnow() - start < 1000000000)
        continue;
    check("waiting", VIRT_REG(VIRT_GF_TTY_BASE + GF_TTY_BYTES_READY) != 0);
    spl0();
    start = rtcnow();
    while (rtcnow() - start < 50000000)         /* 50 ms */
        continue;
    splhigh();
    check("closed", nin == 0);

    open_console();
    start = rtcnow();
    spl0();
    while (nin < INPUT && rtcnow() - start < 1000000000)
        continue;
    splhigh();
    check("open", input_ok());
    report("ttyrxopen");
}
