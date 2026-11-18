/*
 * sys/m68k/vd.c and disklabel.c partitions: the MBR entries, transfers
 * offset into a partition and bounded by its end, missing partitions,
 * and a disk without an MBR (D36).
 */
#include "ttest.h"
#include "vdstub.h"

static char data[2 * DEV_BSIZE];

void
got_signal(int sig)
{
    check("no signal", 0);
}

int
main()
{
    struct diskpart dp;
    struct buf b;

    ttest_init();
    noproc = 1;
    probe_disks();

    check("open vd0a", vdopen(VD0A, 0, 0) == 0);
    check("open vd0b", vdopen(VD0B, 0, 0) == 0);
    check("no vd0c", vdopen(VD0C, 0, 0) == ENXIO);
    bzero(&dp, sizeof(dp));
    check("vd0a entry", vdioctl(VD0A, DIOCGETPART, (caddr_t) &dp, 0) == 0 &&
        dp.dp_type == 0xb7 && dp.dp_offset == PA_START &&
        dp.dp_nsectors == PA_SIZE);
    check("vd0a size", vdsize(VD0A) == PA_SIZE / 2);
    check("vd0b entry", vdioctl(VD0B, DIOCGETPART, (caddr_t) &dp, 0) == 0 &&
        dp.dp_type == 0xb8 && dp.dp_offset == PB_START &&
        dp.dp_nsectors == PB_SIZE);
    check("vd0b size", vdsize(VD0B) == PB_SIZE / 2);

    check("vd0a data", xfer(VD0A, 0, data, DEV_BSIZE, 1) == 0 &&
        sector_ok(data, PA_START) && sector_ok(data + 512, PA_START + 1));
    check("vd0b data", xfer(VD0B, 1, data, DEV_BSIZE, 1) == 0 &&
        sector_ok(data, PB_START + 2));

    /* Past the end of vd0a: EOF at the end, truncated across it. */
    setbuf(&b, VD0A, PA_SIZE / 2, data, DEV_BSIZE, 1);
    vdstrategy(&b);
    biowait(&b);
    spl0();
    check("eof", geterror(&b) == 0 && b.b_resid == DEV_BSIZE);
    setbuf(&b, VD0A, PA_SIZE / 2 - 1, data, 2 * DEV_BSIZE, 1);
    vdstrategy(&b);
    biowait(&b);
    spl0();
    check("truncated", geterror(&b) == 0 && b.b_bcount == DEV_BSIZE &&
        sector_ok(data + 512, PA_START + PA_SIZE - 1));

    /* No MBR on vd1: only the whole disk. */
    check("vd1 raw", vdopen(VD1, 0, 0) == 0);
    check("no vd1a", vdopen(makedev(0, 9), 0, 0) == ENXIO);
    report("vdpart");
}
