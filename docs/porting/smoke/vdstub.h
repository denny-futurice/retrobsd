/*
 * The buffer cache routines sys/m68k/vd.c calls, for the virtio disk
 * tests, and helpers to run transfers.  biowait() sleeps with stop
 * until the disk interrupt calls biodone().  Include in one file per test.
 *
 * run.sh attaches two disks, with a virtio-rng in a slot between them:
 *   vd0  disk A, read-write, DISKA sectors, an MBR in sector 0 with
 *        partition 1 at PA_START and 2 at PB_START
 *   vd1  disk B, read-only, DISKB sectors, no MBR
 * Every sector other than A's sector 0 holds 64 lines "%07d\n" of its
 * sector number.  Reads of A's sector BAD_SECTOR fail.
 */
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/kconfig.h>
#include <sys/disk.h>
#include <sys/dk.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <machine/vd.h>
#include <machine/disklabel.h>

#define DISKA           2048
#define DISKB           1024
#define PA_START        64
#define PA_SIZE         512
#define PB_START        576
#define PB_SIZE         256
#define BAD_SECTOR      2000

#define VD0             makedev(0, 0)
#define VD0A            makedev(0, 1)
#define VD0B            makedev(0, 2)
#define VD0C            makedev(0, 3)
#define VD1             makedev(0, 8)

extern struct driver vddriver;

/* The counters themselves are in kern_clock.c. */
void
dk_alloc(int *dkn, int slots, char *name)
{
    *dkn = dk_n < DK_NDRIVE ? dk_n++ : -1;
}

void
bflush(dev_t dev)
{
}

/* SR of the last biodone(), and how many there were. */
int done_sr = -1, ndone;

void
biodone(struct buf *bp)
{
    __asm__ volatile ("move.w %%sr, %0" : "=d" (done_sr));
    ndone++;
    bp->b_flags |= B_DONE;
}

void
biowait(struct buf *bp)
{
    for (;;) {
        splhigh();
        if (bp->b_flags & B_DONE)
            break;
        __asm__ volatile ("stop #0x2000");      /* wait at level 0 */
    }
}

int
geterror(struct buf *bp)
{
    if (bp->b_flags & B_ERROR)
        return bp->b_error ? bp->b_error : EIO;
    return 0;
}

static struct buf ebuf;
static char ebufdata[DEV_BSIZE];

struct buf *
geteblk()
{
    bzero(&ebuf, sizeof(ebuf));
    ebuf.b_addr = ebufdata;
    ebuf.b_flags = B_BUSY;
    return &ebuf;
}

void
brelse(struct buf *bp)
{
}

/* Probe both units, as autoconfiguration does. */
static void
probe_disks(void)
{
    struct conf_device cd;

    bzero(&cd, sizeof(cd));
    cd.dev_driver = &vddriver;
    cd.dev_unit = 0;
    check("probe vd0", (*vddriver.d_init)(&cd));
    cd.dev_unit = 1;
    check("probe vd1", (*vddriver.d_init)(&cd));
}

/* Set up bp for a transfer of n bytes at 1 KB block blkno. */
static void
setbuf(struct buf *bp, dev_t dev, daddr_t blkno, void *addr, int n, int read)
{
    bzero(bp, sizeof(*bp));
    bp->b_dev = dev;
    bp->b_blkno = blkno;
    bp->b_addr = addr;
    bp->b_bcount = n;
    bp->b_flags = B_BUSY | (read ? B_READ : B_WRITE);
}

/* Transfer and wait; return the error. */
static int
xfer(dev_t dev, daddr_t blkno, void *addr, int n, int read)
{
    struct buf b;

    setbuf(&b, dev, blkno, addr, n, read);
    vdstrategy(&b);
    biowait(&b);
    spl0();
    return geterror(&b);
}

/* The 512 bytes at p are the pattern of sector n. */
static __attribute__((unused)) int
sector_ok(const char *p, u_int n)
{
    char line[8];
    int i;

    for (i = 6; i >= 0; i--, n /= 10)
        line[i] = '0' + n % 10;
    line[7] = '\n';
    for (i = 0; i < 512; i++)
        if (p[i] != line[i % 8])
            return 0;
    return 1;
}
