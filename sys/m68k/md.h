/*
 * Memory disk for swap (D41).
 */
#ifndef _MACHINE_MD_H_
#define _MACHINE_MD_H_

#ifdef KERNEL
struct buf;

int mdopen(dev_t dev, int flag, int mode);
int mdclose(dev_t dev, int flag, int mode);
void mdstrategy(struct buf *bp);
daddr_t mdsize(dev_t dev);
int mdioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif /* _MACHINE_MD_H_ */
