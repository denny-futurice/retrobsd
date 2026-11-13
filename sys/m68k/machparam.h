/*
 * Machine dependent constants for the Motorola 68030 (QEMU virt).
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef ENDIAN

/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define LITTLE          1234            /* least-significant byte first (vax) */
#define BIG             4321            /* most-significant byte first */
#define PDP             3412            /* LSB first in word, MSW first in long (pdp) */
#define ENDIAN          BIG             /* byte order on m68k */

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define MAXSLP          20

/*
 * Clock ticks per second. The HZ value must be an integer factor of 1000.
 */
#ifndef HZ
#define HZ              200
#endif

/*
 * System parameter formulae.
 */
#ifndef NBUF
#define NBUF            10                      /* number of i/o buffers */
#endif
#ifndef MAXUSERS
#define MAXUSERS        1                       /* number of user logins */
#endif
#ifndef NPROC
#define NPROC           25                      /* number of processes */
#endif
#ifndef NINODE
#define NINODE          24
#endif
#ifndef NFILE
#define NFILE           24
#endif
#define NNAMECACHE      (NINODE * 11/10)
#define NCALL           (16 + 2 * MAXUSERS)
#define NCLIST          32                      /* number or CBSIZE blocks */
#ifndef SMAPSIZ
#define SMAPSIZ         NPROC                   /* size of swap allocation map */
#endif

/*
 * Disk blocks.
 */
#define DEV_BSIZE       1024            /* the same as MAXBSIZE */
#define DEV_BSHIFT      10              /* log2(DEV_BSIZE) */
#define DEV_BMASK       (DEV_BSIZE-1)

/* Bytes to disk blocks */
#define btod(x)         (((x) + DEV_BSIZE-1) >> DEV_BSHIFT)

/*
 * Memory map (provisional, D16).  QEMU virt has RAM from address 0;
 * there is no MMU, so every process runs in the same fixed user region
 * and is swapped whole.
 *
 *      0x00000000      vector table, kernel text, data, bss and stack
 *      0x00080000      USER_DATA_START: user text, data, bss, heap
 *      0x000c0000      USER_DATA_END: top of the user stack
 */
#define KERNEL_DATA_START       0x00000000
#define KERNEL_DATA_SIZE        (512*1024)
#define KERNEL_DATA_END         (KERNEL_DATA_START + KERNEL_DATA_SIZE)

#define USER_DATA_SIZE          (256*1024)
#define USER_DATA_START         KERNEL_DATA_END
#define USER_DATA_END           (USER_DATA_START + USER_DATA_SIZE)

#define stacktop(siz)           (USER_DATA_END)
#define stackbas(siz)           (USER_DATA_END-(siz))

/*
 * User area: a user structure, followed by the kernel
 * stack.  The number for USIZE is determined empirically.
 * It is larger than on PIC32 because 68030 exception stack
 * frames are up to 92 bytes.
 */
#define USIZE           4096
#define SSIZE           2048            /* initial stack size (bytes) */

/*
 * Collect kernel statistics by default.
 */
#if !defined(UCB_METER) && !defined(NO_UCB_METER)
#define UCB_METER
#endif

#ifdef KERNEL

/*
 * Status register bits.
 */
#define PSL_T           0x8000          /* trace on every instruction (T1) */
#define PSL_T0          0x4000          /* trace on change of flow */
#define PSL_S           0x2000          /* supervisor mode */
#define PSL_M           0x1000          /* master stack */
#define PSL_IPL         0x0700          /* interrupt priority mask */
#define PSL_IPL7        0x0700          /* all interrupts masked */
#define PSL_CCR         0x001f          /* condition codes X N Z V C */
#define PSL_C           0x0001          /* carry: syscall error (D23) */
#define PSL_LOWIPL      PSL_S           /* supervisor, all interrupts enabled */
#define PSL_HIGHIPL     (PSL_S | PSL_IPL7)
#define PSL_USERSR      (PSL_CCR | PSL_T)       /* bits user code may set (D22) */

/*
 * Macros to decode processor status word.
 */
#define USERMODE(ps)    (((ps) & PSL_S) == 0)
#define BASEPRI(ps)     (((ps) & PSL_IPL) == 0)

/*
 * Set the status register to `s' and return the previous value.
 * As on PIC32 there are only two levels for now: all interrupts
 * masked, or none.
 */
static inline int
_spl(int s)
{
    int sr;

    __asm__ volatile ("movew %%sr,%0; movew %1,%%sr"
        : "=&d" (sr) : "di" (s) : "memory");
    return sr;
}

#define splbio()        _spl(PSL_HIGHIPL)
#define spltty()        _spl(PSL_HIGHIPL)
#define splclock()      _spl(PSL_HIGHIPL)
#define splhigh()       _spl(PSL_HIGHIPL)
#define splnet()        _spl(PSL_LOWIPL)
#define splsoftclock()  _spl(PSL_LOWIPL)
#define spl0()          _spl(PSL_LOWIPL)
#define splx(s)         _spl(s)

#define noop()          __asm__ volatile ("nop")

/*
 * Wait for something to happen.
 */
void idle(void);

/*
 * Microsecond delay routine.
 */
void udelay(unsigned usec);

/*
 * Setup system timer for `hz' timer interrupts per second.
 */
void clkstart(void);

#endif /* KERNEL */

#endif /* ENDIAN */
