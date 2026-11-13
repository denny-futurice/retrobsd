/*
 * Machine dependent code for the Motorola 68030 on QEMU virt.
 * Ported from sys/pic32/machdep.c.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/inode.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/fs.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/reboot.h>
#include <sys/msgbuf.h>
#include <sys/namei.h>
#include <sys/mount.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kconfig.h>
#include <sys/tty.h>
#include <machine/virt.h>
#ifdef GFTTY_ENABLED
#   include <machine/gftty.h>
#endif

int     hz = HZ;
int     usechz = (1000000L + HZ - 1) / HZ;
#ifdef TIMEZONE
struct  timezone tz = { TIMEZONE, DST };
#else
struct  timezone tz = { 8*60, 1 };
#endif
int     nproc = NPROC;

struct  namecache namecache [NNAMECACHE];
char    bufdata [NBUF * MAXBSIZE];
struct  inode inode [NINODE];
struct  callout callout [NCALL];
struct  mount mount [NMOUNT];
struct  buf buf [NBUF], bfreelist [BQUEUES];
struct  bufhd bufhash [BUFHSZ];
struct  cblock cfree [NCLIST];
struct  proc proc [NPROC];
struct  file file [NFILE];

/*
 * Remove the ifdef/endif to run the kernel in unsecure mode even when in
 * a multiuser state.  Normally 'init' raises the security level to 1
 * upon transitioning to multiuser.  Setting the securelevel to -1 prevents
 * the secure level from being raised by init.
 */
#ifdef  PERMANENTLY_INSECURE
int securelevel = -1;
#else
int securelevel = 0;
#endif

struct mapent   swapent[SMAPSIZ];
struct map  swapmap[1] = {
    { swapent,
      &swapent[SMAPSIZ],
      "swapmap" },
};

int waittime = -1;

/* CPU and FPU type bits from the bootinfo records, 0 if absent. */
static unsigned bi_cputype, bi_fputype;

static int
nodump(dev_t dev)
{
    printf("\ndumping to dev %o off %D: not implemented\n", dumpdev, dumplo);
    return 0;
}

int (*dump)(dev_t) = nodump;

dev_t   pipedev;
daddr_t dumplo = (daddr_t) 1024;

/*
 * Walk the bootinfo records QEMU left after the kernel image.
 * This must run before anything uses the memory past _end.
 * Only the first memory chunk counts: virt has one.
 */
static void
read_bootinfo()
{
    extern char _end[];
    struct bi_record *rec;

    rec = (struct bi_record*) (((unsigned) _end + 1) & ~1);
    while (rec->bi_tag != BI_LAST) {
        switch (rec->bi_tag) {
        case BI_CPUTYPE:
            bi_cputype = rec->bi_data[0];
            break;
        case BI_FPUTYPE:
            bi_fputype = rec->bi_data[0];
            break;
        case BI_MEMCHUNK:
            if (physmem == 0)
                physmem = rec->bi_data[1];
            break;
        }
        if (rec->bi_size < sizeof(*rec))
            break;
        rec = (struct bi_record*) ((char*) rec + rec->bi_size);
    }
}

/*
 * Machine dependent startup code.  startup.S has already masked
 * interrupts, set the VBR and cleared .bss; QEMU loaded .data in place.
 */
void
startup()
{
    read_bootinfo();
    if (physmem == 0)
        physmem = USER_DATA_END;

    /*
     * Early setup for console devices.
     */
#if defined(GFTTY_ENABLED) && CONS_MAJOR == UART_MAJOR
    gfttyinit();
#endif

    boothowto = 0;
}

static void
cpuidentify()
{
    printf("cpu: MC68030 (bootinfo cpu %x fpu %x), %u kbytes of RAM\n",
        bi_cputype, bi_fputype, physmem >> 10);
}

/*
 * Check whether the controller has been successfully initialized.
 */
static int
is_controller_alive(struct driver *driver, int unit)
{
    struct conf_ctlr *ctlr;

    /* No controller - that's OK. */
    if (driver == 0)
        return 1;

    for (ctlr = conf_ctlr_init; ctlr->ctlr_driver; ctlr++) {
        if (ctlr->ctlr_driver == driver &&
            ctlr->ctlr_unit == unit &&
            ctlr->ctlr_alive)
        {
            return 1;
        }
    }
    return 0;
}

/*
 * Configure all controllers and devices as specified
 * in the kernel configuration file.
 */
void kconfig()
{
    struct conf_ctlr *ctlr;
    struct conf_device *dev;

    cpuidentify();

    /* Probe and initialize controllers first. */
    for (ctlr = conf_ctlr_init; ctlr->ctlr_driver; ctlr++) {
        if ((*ctlr->ctlr_driver->d_init)(ctlr)) {
            ctlr->ctlr_alive = 1;
        }
    }

    /* Probe and initialize devices. */
    for (dev = conf_device_init; dev->dev_driver; dev++) {
        if (is_controller_alive(dev->dev_cdriver, dev->dev_ctlr)) {
            if ((*dev->dev_driver->d_init)(dev)) {
                dev->dev_alive = 1;
            }
        }
    }
}

/*
 * Sit and wait for something to happen...
 */
void
idle()
{
    /* Indicate that no process is running. */
    noproc = 1;

    /* Set SPL low so we can be interrupted. */
    int x = spl0();

    /* Wait for an interrupt, with all levels enabled. */
    __asm__ volatile ("stop #0x2000");

    /* Restore previous SPL. */
    splx(x);
}

static void
virt_ctrl(unsigned cmd)
{
    VIRT_REG(VIRT_CTRL_BASE + VIRT_CTRL_CMD) = cmd;
}

void
boot(dev_t dev, int howto)
{
    if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
        register struct fs *fp;
        register struct buf *bp;
        int iter, nbusy;

        /*
         * Force the root filesystem's superblock to be updated,
         * so the date will be as current as possible after
         * rebooting.
         */
        fp = getfs(rootdev);
        if (fp)
            fp->fs_fmod = 1;
        waittime = 0;
        printf("syncing disks... ");
        (void) splnet();
        sync();
        for (iter = 0; iter < 20; iter++) {
            nbusy = 0;
            for (bp = &buf[NBUF]; --bp >= buf; )
                if (bp->b_flags & B_BUSY)
                    nbusy++;
            if (nbusy == 0)
                break;
            printf("%d ", nbusy);
            udelay(40000L * iter);
        }
        printf("done\n");
    }
    (void) splhigh();
    if (! (howto & RB_HALT)) {
        if ((howto & RB_DUMP) && dumpdev != NODEV) {
            /*
             * Take a dump of memory by calling (*dump)(),
             * which must correspond to dumpdev.
             * It should dump from dumplo blocks to the end
             * of memory or to the end of the logical device.
             */
            (*dump)(dumpdev);
        }
        /* Restart the machine; QEMU reloads the kernel. */
        virt_ctrl(VIRT_CTRL_CMD_RESET);
    }
    printf("halted\n");

    /* Stop QEMU.  It exits asynchronously, so wait for it. */
    virt_ctrl(VIRT_CTRL_CMD_HALT);
    for (;;)
        __asm__ volatile ("stop #0x2700");
    /*NOTREACHED*/
}

/*
 * Setup system timer for `hz' timer interrupts per second.
 * The clock interrupt is wired up in M3; until then this is a no-op.
 */
void
clkstart()
{
}

/*
 * Microsecond delay routine.
 *
 * Busy-waits on the Goldfish RTC, which counts nanoseconds.  Only the
 * low word is used: the signed difference is valid for waits under 2 s,
 * so longer delays are done in one-second steps.
 */
void
udelay(u_int usec)
{
    volatile unsigned *rtc_low = &VIRT_REG(VIRT_GF_RTC_BASE + GF_RTC_TIME_LOW);
    unsigned final, chunk;

    while (usec > 0) {
        chunk = (usec > 1000000) ? 1000000 : usec;
        usec -= chunk;
        final = *rtc_low + chunk * 1000;

        /* This comparison is valid only when using a signed type. */
        while ((int) (*rtc_low - final) < 0)
            continue;
    }
}

/*
 * Increment user profiling counters.
 */
void addupc(caddr_t pc, struct uprof *pbuf, int ticks)
{
    unsigned indx;

    if (pc < (caddr_t) pbuf->pr_off)
        return;

    indx = pc - (caddr_t) pbuf->pr_off;
    indx = (indx * pbuf->pr_scale) >> 16;
    if (indx >= pbuf->pr_size)
        return;

    pbuf->pr_base[indx] += ticks;
}

/*
 * Find the index of the least significant set bit in the 32-bit word.
 * If LSB bit is set - return 1.
 * If only MSB bit is set - return 32.
 * Return 0 when no bit is set.
 *
 * bfffo numbers bits from the MSB, so isolate the lowest set bit and
 * find its offset from the top.
 */
int
ffs(u_long i)
{
    u_long offset;

    if (i == 0)
        return 0;
    __asm__ ("bfffo %1{#0:#32},%0" : "=d" (offset) : "d" (i & -i));
    return 32 - offset;
}

/*
 * Copy a null terminated string from one point to another.
 * Returns zero on success, ENOENT if maxlength exceeded.
 * If lencopied is non-zero, *lencopied gets the length of the copy
 * (including the null terminating byte).
 */
int
copystr(caddr_t src, caddr_t dest, u_int maxlength, u_int *lencopied)
{
    caddr_t dest0 = dest;
    int error = ENOENT;

    if (maxlength != 0) {
        while ((*dest++ = *src++) != '\0') {
            if (--maxlength == 0) {
                /* Failed. */
                goto done;
            }
        }
        /* Succeeded. */
        error = 0;
    }
done:
    if (lencopied != 0)
        *lencopied = dest - dest0;
    return error;
}

/*
 * Calculate the length of a string.
 */
size_t
strlen(const char *s)
{
    const char *s0 = s;

    while (*s++ != '\0')
        ;
    return s - s0 - 1;
}

/*
 * Return 0 if a user address is valid.
 * There is one memory region allowed for user: the fixed user RAM.
 */
int
baduaddr(caddr_t addr)
{
    if (addr >= (caddr_t) USER_DATA_START &&
        addr < (caddr_t) USER_DATA_END)
        return 0;
    return 1;
}

/*
 * Return 0 if a kernel address is valid.
 * There is only one memory region allowed for kernel: RAM below the
 * user region.
 */
int
badkaddr(caddr_t addr)
{
    if (addr >= (caddr_t) KERNEL_DATA_START &&
        addr < (caddr_t) KERNEL_DATA_END)
        return 0;
    return 1;
}

/*
 * Insert the specified element into a queue immediately after
 * the specified predecessor element.
 */
void insque(void *element, void *predecessor)
{
    struct que {
        struct que *q_next;
        struct que *q_prev;
    };
    register struct que *e = (struct que *) element;
    register struct que *prev = (struct que *) predecessor;

    e->q_prev = prev;
    e->q_next = prev->q_next;
    prev->q_next->q_prev = e;
    prev->q_next = e;
}

/*
 * Remove the specified element from the queue.
 */
void remque(void *element)
{
    struct que {
        struct que *q_next;
        struct que *q_prev;
    };
    register struct que *e = (struct que *) element;

    e->q_prev->q_next = e->q_next;
    e->q_next->q_prev = e->q_prev;
}

/*
 * Compare strings.
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
    register int ret, tmp;

    if (n == 0)
        return 0;
    do {
        ret = *s1++ - (tmp = *s2++);
    } while ((ret == 0) && (tmp != 0) && --n);
    return ret;
}

/* Nonzero if pointer is not aligned on a "sz" boundary.  */
#define UNALIGNED(p, sz)    ((unsigned) (p) & ((sz) - 1))

/*
 * Copy data from the memory region pointed to by src0 to the memory
 * region pointed to by dst0.
 * If the regions overlap, the behavior is undefined.
 */
void
bcopy(const void *src0, void *dst0, size_t nbytes)
{
    unsigned char *dst = dst0;
    const unsigned char *src = src0;
    unsigned *aligned_dst;
    const unsigned *aligned_src;

    /* If the size is small, or either SRC or DST is unaligned,
     * then punt into the byte copy loop.  This should be rare.  */
    if (nbytes >= 4*sizeof(unsigned) &&
        ! UNALIGNED(src, sizeof(unsigned)) &&
        ! UNALIGNED(dst, sizeof(unsigned))) {
        aligned_dst = (unsigned*) dst;
        aligned_src = (const unsigned*) src;

        /* Copy 4X unsigned words at a time if possible.  */
        while (nbytes >= 4*sizeof(unsigned)) {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            nbytes -= 4*sizeof(unsigned);
        }

        /* Copy one unsigned word at a time if possible.  */
        while (nbytes >= sizeof(unsigned)) {
            *aligned_dst++ = *aligned_src++;
            nbytes -= sizeof(unsigned);
        }

        /* Pick up any residual with a byte copier.  */
        dst = (unsigned char*) aligned_dst;
        src = (const unsigned char*) aligned_src;
    }

    while (nbytes--)
        *dst++ = *src++;
}

void *
memcpy(void *dst, const void *src, size_t nbytes)
{
    bcopy(src, dst, nbytes);
    return dst;
}

/*
 * Fill the array with zeroes.
 */
void
bzero(void *dst0, size_t nbytes)
{
    unsigned char *dst;
    unsigned *aligned_dst;

    dst = (unsigned char*) dst0;
    while (UNALIGNED(dst, sizeof(unsigned))) {
        *dst++ = 0;
        if (--nbytes == 0)
            return;
    }
    if (nbytes >= sizeof(unsigned)) {
        /* If we get this far, we know that nbytes is large and dst is word-aligned. */
        aligned_dst = (unsigned*) dst;

        while (nbytes >= 4*sizeof(unsigned)) {
            *aligned_dst++ = 0;
            *aligned_dst++ = 0;
            *aligned_dst++ = 0;
            *aligned_dst++ = 0;
            nbytes -= 4*sizeof(unsigned);
        }
        while (nbytes >= sizeof(unsigned)) {
            *aligned_dst++ = 0;
            nbytes -= sizeof(unsigned);
        }
        dst = (unsigned char*) aligned_dst;
    }

    /* Pick up the remainder with a bytewise loop.  */
    while (nbytes--)
        *dst++ = 0;
}

/*
 * Compare not more than nbytes of data pointed to by m1 with
 * the data pointed to by m2. Return an integer greater than, equal to or
 * less than zero according to whether the object pointed to by
 * m1 is greater than, equal to or less than the object
 * pointed to by m2.
 */
int
bcmp(const void *m1, const void *m2, size_t nbytes)
{
    const unsigned char *s1 = (const unsigned char*) m1;
    const unsigned char *s2 = (const unsigned char*) m2;
    const unsigned *aligned1, *aligned2;

    /* If the size is too small, or either pointer is unaligned,
     * then we punt to the byte compare loop.  Hopefully this will
     * not turn up in inner loops.  */
    if (nbytes >= 4*sizeof(unsigned) &&
        ! UNALIGNED(s1, sizeof(unsigned)) &&
        ! UNALIGNED(s2, sizeof(unsigned))) {
        /* Otherwise, load and compare the blocks of memory one
           word at a time.  */
        aligned1 = (const unsigned*) s1;
        aligned2 = (const unsigned*) s2;
        while (nbytes >= sizeof(unsigned)) {
            if (*aligned1 != *aligned2)
                break;
            aligned1++;
            aligned2++;
            nbytes -= sizeof(unsigned);
        }

        /* check remaining characters */
        s1 = (const unsigned char*) aligned1;
        s2 = (const unsigned char*) aligned2;
    }
    while (nbytes--) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++;
        s2++;
    }
    return 0;
}

int
copyout(caddr_t from, caddr_t to, u_int nbytes)
{
    if (baduaddr(to) || baduaddr(to + nbytes - 1))
        return EFAULT;
    bcopy(from, to, nbytes);
    return 0;
}

int copyin (caddr_t from, caddr_t to, u_int nbytes)
{
    if (baduaddr(from) || baduaddr(from + nbytes - 1))
        return EFAULT;
    bcopy(from, to, nbytes);
    return 0;
}
