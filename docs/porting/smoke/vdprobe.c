/*
 * sys/m68k/vd.c probe: the two disks are found past the virtio-rng
 * between them, numbered in command-line order, with their capacity
 * and read-only flag (D36).
 */
#include "ttest.h"
#include "vdstub.h"

void
got_signal(int sig)
{
    check("no signal", 0);
}

int
main()
{
    struct conf_device cd;
    int size;

    ttest_init();
    noproc = 1;
    probe_disks();
    bzero(&cd, sizeof(cd));
    cd.dev_driver = &vddriver;
    cd.dev_unit = 2;
    check("no vd2", ! (*vddriver.d_init)(&cd));

    check("open vd0", vdopen(VD0, 0, 0) == 0);
    size = 0;
    check("vd0 size", vdioctl(VD0, DIOCGETMEDIASIZE, (caddr_t) &size, 0) == 0 &&
        size == DISKA / 2);
    check("vd0 writable", xfer(VD0, 900, ebufdata, DEV_BSIZE, 0) == 0);
    check("open vd1", vdopen(VD1, 0, 0) == 0);
    size = 0;
    check("vd1 size", vdioctl(VD1, DIOCGETMEDIASIZE, (caddr_t) &size, 0) == 0 &&
        size == DISKB / 2);
    check("vd1 read-only", xfer(VD1, 3, ebufdata, DEV_BSIZE, 0) == EROFS);
    report("vdprobe");
}
