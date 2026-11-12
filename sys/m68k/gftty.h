/*
 * Goldfish TTY driver for QEMU virt.
 */
#ifndef _GFTTY_H
#define _GFTTY_H

#define NGFTTY 1

#ifdef KERNEL

void gfttyinit(void);
int gfttyopen(dev_t dev, int flag, int mode);
int gfttyclose(dev_t dev, int flag, int mode);
int gfttyread(dev_t dev, struct uio *uio, int flag);
int gfttywrite(dev_t dev, struct uio *uio, int flag);
int gfttyselect(dev_t dev, int rw);
int gfttyioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);
void gfttystart(struct tty *tp);
void gfttyputc(dev_t dev, char c);
char gfttygetc(dev_t dev);

extern struct tty gfttyttys[NGFTTY];

#endif

#endif
