/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * Device switch tables for m68k, based on sys/pic32/devsw.c.
 *
 * Major numbers are kept from PIC32, so /dev entries in rootfs.manifest
 * keep their meaning.  Slots for PIC32-only hardware stay as empty
 * entries; the trailing PIC32 hardware majors (10-17) are dropped.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/clist.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/swap.h>

#ifdef PTY_ENABLED
#   include <sys/pty.h>
#endif

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
int nulldev()
{
    return 0;
}

int noopen(dev_t dev, int flag, int mode)
{
    return ENXIO;
}

int norw(dev_t dev, struct uio *uio, int flag)
{
    return 0;
}

int noioctl(dev_t dev, u_int cmd, caddr_t data, int flag)
{
    return EIO;
}

daddr_t nosize(dev_t dev)
{
    return 0;
}

#define NOBDEV \
    noopen,         noopen,         nostrategy, \
    nosize,         noioctl,

const struct bdevsw bdevsw[] = {
{   /* 0 - disk: none until the virtio block driver (M4, D20) */
    NOBDEV
},
{   /* 1 - unused (PIC32 sramc) */
    NOBDEV
},
{   /* 2 - unused (PIC32 sdramp) */
    NOBDEV
},
{   /* 3 - unused (PIC32 mrams) */
    NOBDEV
},
{   /* 4 - swap */
    swopen,         swclose,        swstrategy,
    swsize,         swcioctl,
},
{   /* 5 - unused (PIC32 spirams) */
    NOBDEV
},

{ 0 },
};

const int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]) - 1;

#define NOCDEV \
    noopen,         noopen,         norw,           norw, \
    noioctl,        nulldev,        0,              seltrue, \
    nostrategy,     0,              0,

const struct cdevsw cdevsw[] = {

/*
 * Static drivers - every system has these:
 */
{   /* 0 - console */
    cnopen,         cnclose,        cnread,         cnwrite,
    cnioctl,        nulldev,        cnttys,         cnselect,
    nostrategy,     0,              0,
},
{   /* 1 - mem, kmem, null, zero */
#if MEM_MAJOR != 1
#   error Wrong MEM_MAJOR value!
#endif
    nulldev,        nulldev,        mmrw,           mmrw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     0,              0,
},
{   /* 2 - tty */
    syopen,         nulldev,        syread,         sywrite,
    syioctl,        nulldev,        0,              syselect,
    nostrategy,     0,              0,
},
{   /* 3 - fd */
    fdopen,         nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     0,              0,
},
{   /* 4 - temp (temporary allocation in swap space) */
    swcopen,        swcclose,       swcread,        swcwrite,
    swcioctl,       nulldev,        0,              seltrue,
    nostrategy,     0,              0,
},

/*
 * Optional drivers from here on:
 */
{   /* 5 - log */
#ifdef LOG_ENABLED
    logopen,        logclose,       logread,        norw,
    logioctl,       nulldev,        0,              logselect,
    nostrategy,     0,              0,
#else
    NOCDEV
#endif
},
{   /* 6 - serial tty: Goldfish TTY (M2) */
#if UART_MAJOR != 6
#   error Wrong UART_MAJOR value!
#endif
    NOCDEV
},
{   /* 7 - unused (PIC32 USB serial) */
    NOCDEV
},
{   /* 8, 9 - pty */
#ifdef PTY_ENABLED
    ptsopen,        ptsclose,       ptsread,        ptswrite,
    ptyioctl,       nulldev,        pt_tty,         ptcselect,
    nostrategy,     0,              0,
}, {
    ptcopen,        ptcclose,       ptcread,        ptcwrite,
    ptyioctl,       nulldev,        pt_tty,         ptcselect,
    nostrategy,     0,              0,
#else
    NOCDEV }, { NOCDEV
#endif
},

/*
 * End the list with a blank entry
 */
{ 0 },
};

const int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]) - 1;

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 */
int
iskmemdev(dev_t dev)
{
    if (major(dev) == MEM_MAJOR && (minor(dev) == 0 || minor(dev) == 1))
        return 1;
    return 0;
}

/*
 * Routine to determine if a device is a disk.
 */
int
isdisk(dev_t dev, int type)
{
    if (type != IFBLK)
        return 0;

    switch (major(dev)) {
    case 0:                 /* disk */
    case 4:                 /* sw */
        return 1;
    default:
        return 0;
    }
}

/*
 * Routine to convert from character to block device number.
 */
int
chrtoblk(dev_t dev)
{
    return NODEV;
}
