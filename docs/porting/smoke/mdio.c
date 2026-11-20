/*
 * sys/m68k/md.c, the memory disk for swap (D41): md0 covers the RAM from
 * USER_DATA_END to the bootinfo memory size (run.sh starts QEMU with
 * -m 16M), transfers any byte count both ways at the right address and
 * finish at once, and the bounds give EOF, truncation and EINVAL.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/kconfig.h>
#include <sys/errno.h>
#include <machine/md.h>
#include "ktest.h"

#define MD0             makedev(1, 0)
#define NBLK            ((16 * 1024 * 1024 - USER_DATA_END) / DEV_BSIZE)
#define DISK(blk)       ((u_char *) USER_DATA_END + (blk) * DEV_BSIZE)

extern struct driver mddriver;

static int ndone;
static u_char data[2 * DEV_BSIZE];

void
biodone(struct buf *bp)
{
    ndone++;
    bp->b_flags |= B_DONE;
}

/* Run a transfer of n bytes at block blkno; return the buf. */
static struct buf *
xfer(dev_t dev, daddr_t blkno, void *addr, int n, int read)
{
    static struct buf b;

    bzero(&b, sizeof(b));
    b.b_dev = dev;
    b.b_blkno = blkno;
    b.b_addr = addr;
    b.b_bcount = n;
    b.b_flags = B_BUSY | (read ? B_READ : B_WRITE);
    ndone = 0;
    mdstrategy(&b);
    check("finished at once", ndone == 1 && (b.b_flags & B_DONE));
    return &b;
}

/* p[0..n) is the pattern starting at seed. */
static int
pattern(const u_char *p, int n, int seed)
{
    int i;

    for (i = 0; i < n; i++)
        if (p[i] != (u_char) (seed + 7 * i))
            return 0;
    return 1;
}

static void
fill(u_char *p, int n, int seed)
{
    int i;

    for (i = 0; i < n; i++)
        p[i] = seed + 7 * i;
}

int
main(void)
{
    struct conf_device cd;
    struct buf *bp;

    startup();
    check("no md0 before the probe", mdopen(MD0, 0, 0) == ENXIO);
    bzero(&cd, sizeof(cd));
    cd.dev_driver = &mddriver;
    cd.dev_unit = 1;
    check("no md1", (*mddriver.d_init)(&cd) == 0);
    cd.dev_unit = 0;
    check("probe md0", (*mddriver.d_init)(&cd));
    check("open md0", mdopen(MD0, 0, 0) == 0);
    check("no other minor", mdopen(makedev(1, 1), 0, 0) == ENXIO);
    check("size", mdsize(MD0) == NBLK);

    /* A write of 700 bytes lands at block 3, and nothing past it. */
    fill(DISK(3), DEV_BSIZE, 0x55);
    fill(data, 700, 1);
    bp = xfer(MD0, 3, data, 700, 0);
    check("write ok", !(bp->b_flags & B_ERROR) && bp->b_resid == 0);
    check("written", pattern(DISK(3), 700, 1));
    check("rest untouched", pattern(DISK(3) + 700, DEV_BSIZE - 700, 0x55 + 7 * 700));

    /* A read of 1500 bytes from block 3 into a buffer, nothing past it. */
    fill(DISK(4), DEV_BSIZE, 9);
    fill(data, sizeof(data), 0x33);
    bp = xfer(MD0, 3, data, 1500, 1);
    check("read ok", !(bp->b_flags & B_ERROR) && bp->b_resid == 0);
    check("read block 3", pattern(data, 700, 1));
    check("read block 4", pattern(data + DEV_BSIZE, 1500 - DEV_BSIZE, 9));
    check("buffer past count untouched",
        pattern(data + 1500, sizeof(data) - 1500, 0x33 + 7 * 1500));

    /* The last block works; at the end is EOF; across it truncates. */
    fill(data, DEV_BSIZE, 2);
    bp = xfer(MD0, NBLK - 1, data, DEV_BSIZE, 0);
    check("last block", !(bp->b_flags & B_ERROR) && pattern(DISK(NBLK - 1), DEV_BSIZE, 2));
    bp = xfer(MD0, NBLK, data, DEV_BSIZE, 1);
    check("EOF at the end", !(bp->b_flags & B_ERROR) && bp->b_resid == DEV_BSIZE);
    bp = xfer(MD0, NBLK - 1, data, 2 * DEV_BSIZE, 1);
    check("truncated", !(bp->b_flags & B_ERROR) && bp->b_bcount == DEV_BSIZE);
    bp = xfer(MD0, NBLK + 1, data, DEV_BSIZE, 1);
    check("past the end", (bp->b_flags & B_ERROR) && bp->b_error == EINVAL);
    bp = xfer(makedev(1, 1), 0, data, DEV_BSIZE, 1);
    check("bad minor", (bp->b_flags & B_ERROR) && bp->b_error == ENXIO);

    report("mdio");
    return 0;
}
