/*
 * virtio block disk on QEMU virt (D36).
 *
 * The first probe scans the 128 virtio-mmio slots from the top, where QEMU
 * puts the first -device, so vd0 is the first disk on the command line.
 * I/O is interrupt driven, one request per disk at a time: vdstrategy()
 * queues a buffer, vdstart() hands it to the device and vdintr() finishes
 * it.  The device is virtio 1.0, so every register, ring entry and
 * configuration value is little-endian.
 *
 * Minor numbers, as for PIC32's sd:
 *   0     vd0   the whole disk
 *   1-4   vd0a  MBR partitions (sys/m68k/disklabel.c)
 *   8     vd1   the whole second disk, 9-12 its partitions
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/dk.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/kconfig.h>
#include <machine/virt.h>
#include <machine/virtio.h>
#include <machine/disklabel.h>
#include <machine/vd.h>

#define NVD             2
#define vdunit(dev)     ((minor(dev) & 8) >> 3)
#define vdpart(dev)     (minor(dev) & 7)

#define QSIZE           4               /* descriptors: header, data, status */

#define VD_READ(du, off)        virtio_read((du)->base + (off))
#define VD_WRITE(du, off, v)    virtio_write((du)->base + (off), v)

struct vdisk {
    u_int   base;                       /* 0: no such disk */
    int     level, src;                 /* PIC interrupt */
    int     ro;                         /* read-only */
    int     labelled;                   /* part[1..] read from the MBR */
    u_int   openpart;                   /* open partitions */
    int     dkindex;
    struct diskpart part[NPARTITIONS+1];
    struct buf *actf, *actl;            /* queue */
    struct buf *active;                 /* the request on the device */
    u_short avail_idx, used_idx;

    struct vring_desc desc[QSIZE] __attribute__((aligned(16)));
    struct {
        u_short flags, idx, ring[QSIZE], event;
    } avail __attribute__((aligned(2)));
    struct {
        u_short flags, idx;
        struct {
            u_int id, len;
        } ring[QSIZE];
        u_short event;
    } used __attribute__((aligned(4)));
    struct virtio_blk_req req;
    u_char  status;
};

static struct vdisk vddrives[NVD];

/*
 * Hand the first queued buffer to the device.  Called at splbio.
 */
static void
vdstart(struct vdisk *du)
{
    struct buf *bp = du->actf;

    if (bp == 0 || du->active)
        return;
    du->actf = bp->av_forw;
    du->active = bp;
#ifdef UCB_METER
    if (du->dkindex >= 0) {
        dk_busy |= 1 << du->dkindex;
        dk_xfer[du->dkindex]++;
        dk_bytes[du->dkindex] += bp->b_bcount;
    }
#endif
    du->req.type = le32((bp->b_flags & B_READ) ? VIRTIO_BLK_T_IN :
        VIRTIO_BLK_T_OUT);
    du->req.reserved = 0;
    du->req.sector_lo = le32(bp->b_cylin);      /* set by vdstrategy */
    du->req.sector_hi = 0;
    du->status = 0xff;

    du->desc[0].addr_lo = le32((u_int) &du->req);
    du->desc[0].len = le32(sizeof(du->req));
    du->desc[0].flags = le16(VRING_DESC_F_NEXT);
    du->desc[0].next = le16(1);
    du->desc[1].addr_lo = le32((u_int) bp->b_addr);
    du->desc[1].len = le32(bp->b_bcount);
    du->desc[1].flags = le16(VRING_DESC_F_NEXT |
        ((bp->b_flags & B_READ) ? VRING_DESC_F_WRITE : 0));
    du->desc[1].next = le16(2);
    du->desc[2].addr_lo = le32((u_int) &du->status);
    du->desc[2].len = le32(1);
    du->desc[2].flags = le16(VRING_DESC_F_WRITE);
    du->desc[2].next = 0;

    du->avail.ring[du->avail_idx % QSIZE] = 0;
    du->avail_idx++;
    du->avail.idx = le16(du->avail_idx);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

/*
 * Interrupt from PIC level, source src.  Return 0 if no disk uses it.
 */
int
vdintr(int level, int src)
{
    struct vdisk *du;
    struct buf *bp;
    int unit, found = 0;

    for (unit = 0; unit < NVD; unit++) {
        du = &vddrives[unit];
        if (du->base == 0 || du->level != level || du->src != src)
            continue;
        found = 1;
        VD_WRITE(du, VIRTIO_MMIO_INTERRUPT_ACK,
            VD_READ(du, VIRTIO_MMIO_INTERRUPT_STATUS));
        while (du->used_idx != le16(du->used.idx) && du->active) {
            du->used_idx++;
            bp = du->active;
            du->active = 0;
#ifdef UCB_METER
            if (du->dkindex >= 0)
                dk_busy &= ~(1 << du->dkindex);
#endif
            if (du->status == VIRTIO_BLK_S_OK) {
                bp->b_resid = 0;
            } else {
                bp->b_error = EIO;
                bp->b_flags |= B_ERROR;
                bp->b_resid = bp->b_bcount;
            }
            biodone(bp);
            vdstart(du);
        }
    }
    return found;
}

void
vdstrategy(struct buf *bp)
{
    struct vdisk *du = &vddrives[vdunit(bp->b_dev)];
    struct diskpart *p = &du->part[vdpart(bp->b_dev)];
    long part_size = p->dp_nsectors >> 1;
    long nblk = btod(bp->b_bcount);
    int s;

    if (du->base == 0 || vdpart(bp->b_dev) > NPARTITIONS) {
        bp->b_error = ENXIO;
        goto bad;
    }
    if (bp->b_bcount % VIRTIO_BLK_SECTOR) {
        bp->b_error = EINVAL;
        goto bad;
    }
    /*
     * Make sure the transfer is within the partition: return an EOF
     * exactly at its end, or truncate if part of it fits.
     */
    if (bp->b_blkno + nblk > part_size) {
        if (bp->b_blkno == part_size) {
            bp->b_resid = bp->b_bcount;
            biodone(bp);
            return;
        }
        nblk = part_size - bp->b_blkno;
        if (nblk <= 0) {
            bp->b_error = EINVAL;
            goto bad;
        }
        bp->b_bcount = nblk << DEV_BSHIFT;
    }
    bp->b_cylin = p->dp_offset + bp->b_blkno * (DEV_BSIZE / VIRTIO_BLK_SECTOR);
    if (! (bp->b_flags & B_READ) && (bp->b_cylin == 0 || du->ro)) {
        /* Writes to the partition table, or a read-only disk. */
        bp->b_error = EROFS;
        goto bad;
    }

    s = splbio();
    bp->av_forw = 0;
    if (du->actf)
        du->actl->av_forw = bp;
    else
        du->actf = bp;
    du->actl = bp;
    vdstart(du);
    splx(s);
    return;

bad:
    bp->b_flags |= B_ERROR;
    biodone(bp);
}

/*
 * Read the partition table from sector 0.
 */
static int
vdlabel(dev_t dev)
{
    struct vdisk *du = &vddrives[vdunit(dev)];
    struct buf *bp;
    int error;

    bp = geteblk();
    bp->b_dev = makedev(major(dev), vdunit(dev) << 3 | RAWPART);
    bp->b_blkno = 0;
    bp->b_bcount = DEV_BSIZE;
    bp->b_flags |= B_READ;
    vdstrategy(bp);
    biowait(bp);
    error = geterror(bp);
    if (error == 0) {
        mbr_parse((u_char *) bp->b_addr, du->part);
        du->labelled = 1;
    }
    bp->b_flags |= B_INVAL;
    brelse(bp);
    return error;
}

int
vdopen(dev_t dev, int flag, int mode)
{
    int unit = vdunit(dev);
    int part = vdpart(dev);
    struct vdisk *du = &vddrives[unit];
    int error;

    if (unit >= NVD || part > NPARTITIONS || du->base == 0)
        return ENXIO;
    if (! du->labelled) {
        error = vdlabel(dev);
        if (error)
            return error;
    }
    if (part != RAWPART && du->part[part].dp_nsectors == 0)
        return ENXIO;
    du->openpart |= 1 << part;
    return 0;
}

int
vdclose(dev_t dev, int flag, int mode)
{
    struct vdisk *du = &vddrives[vdunit(dev)];

    du->openpart &= ~(1 << vdpart(dev));
    if (du->openpart == 0)
        du->labelled = 0;               /* read the MBR again on next open */
    return 0;
}

/*
 * Partition size in kbytes, 0 if the disk is not open.
 */
daddr_t
vdsize(dev_t dev)
{
    struct vdisk *du = &vddrives[vdunit(dev)];

    if (vdunit(dev) >= NVD || vdpart(dev) > NPARTITIONS || du->openpart == 0)
        return 0;
    return du->part[vdpart(dev)].dp_nsectors >> 1;
}

int
vdioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    struct vdisk *du = &vddrives[vdunit(dev)];
    struct diskpart *dp = &du->part[vdpart(dev)];
    int i;

    switch (cmd) {
    case DIOCGETMEDIASIZE:
        *(int *) addr = dp->dp_nsectors >> 1;
        return 0;
    case DIOCGETPART:
        *(struct diskpart *) addr = *dp;
        return 0;
    case DIOCREINIT:
        for (i = 0; i <= NPARTITIONS; i++)
            bflush(makedev(major(dev), vdunit(dev) << 3 | i));
        return vdlabel(dev);
    }
    return EINVAL;
}

/*
 * Set up the device in slot n as disk du.  Return 0 if it cannot be used.
 */
static int
vdinit(struct vdisk *du, int n)
{
    u_int features;

    du->base = VIRT_VIRTIO_BASE + n * VIRT_VIRTIO_SIZE;
    du->level = VIRT_VIRTIO_LEVEL(n);
    du->src = VIRT_VIRTIO_SRC(n);
    VD_WRITE(du, VIRTIO_MMIO_STATUS, 0);
    VD_WRITE(du, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    VD_WRITE(du, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE |
        VIRTIO_STATUS_DRIVER);

    VD_WRITE(du, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
    if (! (VD_READ(du, VIRTIO_MMIO_DEVICE_FEATURES) & VIRTIO_F_VERSION_1_HI))
        goto fail;
    VD_WRITE(du, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
    features = VD_READ(du, VIRTIO_MMIO_DEVICE_FEATURES) & VIRTIO_BLK_F_RO;
    du->ro = features != 0;
    VD_WRITE(du, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
    VD_WRITE(du, VIRTIO_MMIO_DRIVER_FEATURES, features);
    VD_WRITE(du, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
    VD_WRITE(du, VIRTIO_MMIO_DRIVER_FEATURES, VIRTIO_F_VERSION_1_HI);
    VD_WRITE(du, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE |
        VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
    if (! (VD_READ(du, VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK))
        goto fail;

    VD_WRITE(du, VIRTIO_MMIO_QUEUE_SEL, 0);
    if (VD_READ(du, VIRTIO_MMIO_QUEUE_NUM_MAX) < QSIZE)
        goto fail;
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_NUM, QSIZE);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_DESC_LOW, (u_int) du->desc);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_DESC_HIGH, 0);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_DRIVER_LOW, (u_int) &du->avail);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_DRIVER_HIGH, 0);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_DEVICE_LOW, (u_int) &du->used);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_DEVICE_HIGH, 0);
    VD_WRITE(du, VIRTIO_MMIO_QUEUE_READY, 1);
    VD_WRITE(du, VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE |
        VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK |
        VIRTIO_STATUS_DRIVER_OK);

    /* Capacity in 512-byte sectors; the high word must be 0. */
    if (VD_READ(du, VIRTIO_MMIO_CONFIG + 4) != 0)
        du->part[RAWPART].dp_nsectors = 0xffffffff;
    else
        du->part[RAWPART].dp_nsectors = VD_READ(du, VIRTIO_MMIO_CONFIG);

    VIRT_REG(GF_PIC_ADDR(du->level, GF_PIC_ENABLE)) = GF_PIC_BIT(du->src);
    return 1;
fail:
    VD_WRITE(du, VIRTIO_MMIO_STATUS, 0);
    du->base = 0;
    return 0;
}

/*
 * Find the block devices on the first probe.
 */
static void
vdscan()
{
    static int scanned;
    u_int base;
    int n, unit = 0;

    if (scanned)
        return;
    scanned = 1;
    for (n = VIRT_VIRTIO_SLOTS - 1; n >= 0 && unit < NVD; n--) {
        base = VIRT_VIRTIO_BASE + n * VIRT_VIRTIO_SIZE;
        if (virtio_read(base + VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MMIO_MAGIC ||
            virtio_read(base + VIRTIO_MMIO_VERSION) != 2 ||
            virtio_read(base + VIRTIO_MMIO_DEVICE_ID) != VIRTIO_ID_BLOCK)
            continue;
        if (vdinit(&vddrives[unit], n))
            unit++;
    }
}

static int
vdprobe(void *arg)
{
    struct conf_device *config = arg;
    int unit = config->dev_unit;
    struct vdisk *du = &vddrives[unit];

    if (unit < 0 || unit >= NVD)
        return 0;
    vdscan();
    if (du->base == 0)
        return 0;
    printf("vd%d: at %08x, %u kbytes%s\n", unit, du->base,
        du->part[RAWPART].dp_nsectors / 2, du->ro ? ", read-only" : "");
#ifdef UCB_METER
    dk_alloc(&du->dkindex, 1, unit == 0 ? "vd0" : "vd1");
#endif
    return 1;
}

struct driver vddriver = {
    "vd", vdprobe,
};
