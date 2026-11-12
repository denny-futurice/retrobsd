/*
 * Goldfish TTY driver for QEMU virt, the serial tty major (UART_MAJOR).
 * Based on sys/pic32/uart.c.
 *
 * Polled for now: output is written a byte at a time as soon as it is
 * queued, and input is only read by gfttygetc().  The receive interrupt
 * (Goldfish PIC 1, source 32) is wired up with the interrupt code in M3;
 * until then a read() on the tty sleeps forever.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/kconfig.h>
#include <machine/virt.h>
#include <machine/gftty.h>

#define GFTTY_REG(r)    VIRT_REG(VIRT_GF_TTY_BASE + (r))

struct tty gfttyttys[NGFTTY];

/*
 * Early setup, before the console is used: keep the device interrupt off.
 */
void
gfttyinit()
{
    GFTTY_REG(GF_TTY_CMD) = GF_TTY_CMD_INT_DISABLE;
}

int
gfttyopen(dev_t dev, int flag, int mode)
{
    register struct tty *tp;
    register int unit = minor(dev);

    if (unit >= NGFTTY)
        return (ENXIO);

    tp = &gfttyttys[unit];
    if (! tp->t_addr)
        return (ENXIO);

    tp->t_oproc = gfttystart;
    if ((tp->t_state & TS_ISOPEN) == 0) {
        if (tp->t_ispeed == 0) {
            tp->t_ispeed = B115200;
            tp->t_ospeed = B115200;
        }
        ttychars(tp);
        tp->t_state = TS_ISOPEN | TS_CARR_ON;
        tp->t_flags = ECHO | XTABS | CRMOD | CRTBS | CRTERA | CTLECH | CRTKIL;
    }
    if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
        return (EBUSY);

    return ttyopen(dev, tp);
}

/*ARGSUSED*/
int
gfttyclose(dev_t dev, int flag, int mode)
{
    register struct tty *tp = &gfttyttys[minor(dev)];

    if (! tp->t_addr)
        return ENODEV;

    ttywflush(tp);
    ttyclose(tp);
    return 0;
}

/*ARGSUSED*/
int
gfttyread(dev_t dev, struct uio *uio, int flag)
{
    register struct tty *tp = &gfttyttys[minor(dev)];

    if (! tp->t_addr)
        return ENODEV;

    return ttread(tp, uio, flag);
}

/*ARGSUSED*/
int
gfttywrite(dev_t dev, struct uio *uio, int flag)
{
    register struct tty *tp = &gfttyttys[minor(dev)];

    if (! tp->t_addr)
        return ENODEV;

    return ttwrite(tp, uio, flag);
}

int
gfttyselect(dev_t dev, int rw)
{
    register struct tty *tp = &gfttyttys[minor(dev)];

    if (! tp->t_addr)
        return ENODEV;

    return ttyselect(tp, rw);
}

/*ARGSUSED*/
int
gfttyioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    register struct tty *tp = &gfttyttys[minor(dev)];
    register int error;

    if (! tp->t_addr)
        return ENODEV;

    error = ttioctl(tp, cmd, addr, flag);
    if (error < 0)
        error = ENOTTY;
    return error;
}

/*
 * Start output.  The device accepts a byte at any time, so drain the
 * whole output queue at once; the tty is never left busy.
 */
void
gfttystart(register struct tty *tp)
{
    register int s;

    if (! tp->t_addr)
        return;

    s = spltty();
    if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
        splx(s);
        return;
    }
    while (tp->t_outq.c_cc > 0)
        GFTTY_REG(GF_TTY_PUT_CHAR) = getc(&tp->t_outq) & 0xff;
    ttyowake(tp);
    splx(s);
}

/*
 * Console output, used by cnputc().  Works before the probe.
 */
/*ARGSUSED*/
void
gfttyputc(dev_t dev, char c)
{
    GFTTY_REG(GF_TTY_PUT_CHAR) = (unsigned char) c;
}

/*
 * Console input, used by cngetc(): wait for a byte and read it.
 * The device copies input into memory; with no MMU the kernel
 * address of the buffer is its physical address.
 */
/*ARGSUSED*/
char
gfttygetc(dev_t dev)
{
    static volatile char c;
    int s;

    s = spltty();
    while (GFTTY_REG(GF_TTY_BYTES_READY) == 0)
        continue;
    GFTTY_REG(GF_TTY_DATA_PTR) = (unsigned) &c;
    GFTTY_REG(GF_TTY_DATA_LEN) = 1;
    GFTTY_REG(GF_TTY_CMD) = GF_TTY_CMD_READ_BUFFER;
    splx(s);
    return c;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
gfttyprobe(void *arg)
{
    struct conf_device *config = arg;
    int unit = config->dev_unit;

    if (unit < 0 || unit >= NGFTTY)
        return 0;

    printf("gftty%d: at %08x", unit, VIRT_GF_TTY_BASE);
    if (CONS_MAJOR == UART_MAJOR && CONS_MINOR == unit)
        printf(", console");
    printf("\n");

    gfttyttys[unit].t_addr = (caddr_t) VIRT_GF_TTY_BASE;
    gfttyinit();
    return 1;
}

struct driver gfttydriver = {
    "gftty", gfttyprobe,
};
