/*
 * sys/m68k/vd.c reads: the request completes by interrupt, single and
 * multi-block, several queued at once, a device error, and EOF at the
 * end of the disk (D36).
 */
#include "ttest.h"
#include "vdstub.h"

static char data[3][4 * DEV_BSIZE];

void
got_signal(int sig)
{
    check("no signal", 0);
}

/* p holds n sectors from sector first on. */
static int
sectors_ok(const char *p, u_int first, int n)
{
    int i;

    for (i = 0; i < n; i++)
        if (! sector_ok(p + i * 512, first + i))
            return 0;
    return 1;
}

int
main()
{
    struct buf b[3];
    int i;

    ttest_init();
    noproc = 1;
    probe_disks();
    check("open", vdopen(VD0, 0, 0) == 0 && vdopen(VD1, 0, 0) == 0);

    /* Started by strategy, finished by the interrupt. */
    setbuf(&b[0], VD0, 1, data[0], DEV_BSIZE, 1);
    ndone = 0;
    vdstrategy(&b[0]);
    check("queued", ! (b[0].b_flags & B_DONE) && ndone == 0);
    biowait(&b[0]);
    spl0();
    check("one block", geterror(&b[0]) == 0 && b[0].b_resid == 0 &&
        sectors_ok(data[0], 2, 2));
    check("in interrupt", (done_sr & PSL_IPL) == PSL_IPL);

    check("4 blocks", xfer(VD0, 100, data[1], 4 * DEV_BSIZE, 1) == 0 &&
        sectors_ok(data[1], 200, 8));
    check("vd1", xfer(VD1, 0, data[2], DEV_BSIZE, 1) == 0 &&
        sectors_ok(data[2], 0, 2));

    /* Three at once: one on the device, two waiting. */
    bzero(data, sizeof(data));
    ndone = 0;
    splbio();
    for (i = 0; i < 3; i++) {
        setbuf(&b[i], VD0, 10 * i + 5, data[i], 2 * DEV_BSIZE, 1);
        vdstrategy(&b[i]);
    }
    for (i = 0; i < 3; i++)
        biowait(&b[i]);
    spl0();
    check("queue", ndone == 3);
    for (i = 0; i < 3; i++)
        check("queued data", geterror(&b[i]) == 0 &&
            sectors_ok(data[i], 20 * i + 10, 4));

    /* The device fails the request. */
    setbuf(&b[0], VD0, BAD_SECTOR / 2, data[0], DEV_BSIZE, 1);
    vdstrategy(&b[0]);
    biowait(&b[0]);
    spl0();
    check("error", geterror(&b[0]) == EIO && b[0].b_resid == DEV_BSIZE);
    check("after error", xfer(VD0, BAD_SECTOR / 2 - 1, data[0], DEV_BSIZE, 1) == 0 &&
        sectors_ok(data[0], BAD_SECTOR - 2, 2));

    /* At the end of the disk: EOF, no error. */
    setbuf(&b[0], VD0, DISKA / 2, data[0], DEV_BSIZE, 1);
    vdstrategy(&b[0]);
    biowait(&b[0]);
    spl0();
    check("eof", geterror(&b[0]) == 0 && b[0].b_resid == DEV_BSIZE);
    report("vdread");
}
