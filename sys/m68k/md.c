/*
 * Memory disk for swap (D41).
 *
 * md0 is the physical memory above the user window, from USER_DATA_END to
 * the end of RAM (physmem, from the bootinfo).  The strategy routine copies
 * with bcopy() and finishes the request at once, so swap I/O never waits.
 * Any byte count is accepted, as by vd: swap moves the u area and segments
 * of arbitrary size.  There is one unit and no partitions: minor 0 only.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/kconfig.h>
#include <sys/errno.h>
#include <machine/md.h>

#define MD_START        USER_DATA_END

static long mdkbytes;                   /* size in DEV_BSIZE blocks, 0 if none */

void
mdstrategy(struct buf *bp)
{
    long nblk = btod(bp->b_bcount);
    caddr_t addr;

    if (minor(bp->b_dev) != 0 || mdkbytes == 0) {
        bp->b_error = ENXIO;
        goto bad;
    }
    /*
     * Make sure the transfer is within the disk: return an EOF exactly
     * at its end, or truncate if part of it fits.
     */
    if (bp->b_blkno + nblk > mdkbytes) {
        if (bp->b_blkno == mdkbytes) {
            bp->b_resid = bp->b_bcount;
            biodone(bp);
            return;
        }
        nblk = mdkbytes - bp->b_blkno;
        if (nblk <= 0) {
            bp->b_error = EINVAL;
            goto bad;
        }
        bp->b_bcount = nblk << DEV_BSHIFT;
    }
    addr = (caddr_t) MD_START + (bp->b_blkno << DEV_BSHIFT);
    if (bp->b_flags & B_READ)
        bcopy(addr, bp->b_addr, bp->b_bcount);
    else
        bcopy(bp->b_addr, addr, bp->b_bcount);
    bp->b_resid = 0;
    biodone(bp);
    return;

bad:
    bp->b_flags |= B_ERROR;
    biodone(bp);
}

int
mdopen(dev_t dev, int flag, int mode)
{
    if (minor(dev) != 0 || mdkbytes == 0)
        return ENXIO;
    return 0;
}

int
mdclose(dev_t dev, int flag, int mode)
{
    return 0;
}

/*
 * Size in kbytes.
 */
daddr_t
mdsize(dev_t dev)
{
    if (minor(dev) != 0)
        return 0;
    return mdkbytes;
}

int
mdioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    return EINVAL;
}

static int
mdprobe(void *arg)
{
    struct conf_device *config = arg;

    if (config->dev_unit != 0 || physmem <= MD_START + DEV_BSIZE)
        return 0;
    mdkbytes = (physmem - MD_START) >> DEV_BSHIFT;
    printf("md0: at %08x, %u kbytes\n", MD_START, (u_int) mdkbytes);
    return 1;
}

struct driver mddriver = {
    "md", mdprobe,
};
