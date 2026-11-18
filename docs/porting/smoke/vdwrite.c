/*
 * sys/m68k/vd.c writes: data reaches the disk, through the raw disk and
 * a partition; sector 0 and a read-only disk refuse writes (D36).
 */
#include "ttest.h"
#include "vdstub.h"

static char out[DEV_BSIZE], in[DEV_BSIZE];

void
got_signal(int sig)
{
    check("no signal", 0);
}

int
main()
{
    int i;

    ttest_init();
    noproc = 1;
    probe_disks();
    check("open", vdopen(VD0, 0, 0) == 0 && vdopen(VD0A, 0, 0) == 0 &&
        vdopen(VD1, 0, 0) == 0);

    for (i = 0; i < DEV_BSIZE; i++)
        out[i] = i * 7 + 3;
    check("write", xfer(VD0, 900, out, DEV_BSIZE, 0) == 0);
    check("read back", xfer(VD0, 900, in, DEV_BSIZE, 1) == 0 &&
        bcmp(in, out, DEV_BSIZE) == 0);

    /* Block 0 of vd0a is sectors PA_START and PA_START + 1. */
    out[0] ^= 0xff;
    check("write vd0a", xfer(VD0A, 0, out, DEV_BSIZE, 0) == 0);
    check("read back raw", xfer(VD0, PA_START / 2, in, DEV_BSIZE, 1) == 0 &&
        bcmp(in, out, DEV_BSIZE) == 0);

    check("sector 0", xfer(VD0, 0, out, DEV_BSIZE, 0) == EROFS);
    check("label kept", xfer(VD0, 0, in, DEV_BSIZE, 1) == 0 &&
        (u_char) in[510] == 0x55 && (u_char) in[511] == 0xaa);
    check("read-only disk", xfer(VD1, 3, out, DEV_BSIZE, 0) == EROFS);
    check("read-only data", xfer(VD1, 3, in, DEV_BSIZE, 1) == 0 &&
        sector_ok(in, 6));
    report("vdwrite");
}
