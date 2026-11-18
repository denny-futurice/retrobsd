/*
 * Console input by interrupt (sys/m68k/gftty.c, trap.c): after open, the
 * Goldfish TTY receive interrupt passes all input to ttyinput() in order,
 * more than the device's 128-byte buffer holds at once.  The clock runs,
 * as in the kernel: QEMU refills the device buffer only when its main
 * loop wakes, which the tick timer makes it do (D33).
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
    clkstart();
    open_console();
    start = rtcnow();
    spl0();
    while (nin < INPUT && rtcnow() - start < 1000000000)
        continue;
    splhigh();
    check("input", input_ok());
    check("interrupt", inipl == (PSL_HIGHIPL & PSL_IPL));
    report("ttyrx");
}
