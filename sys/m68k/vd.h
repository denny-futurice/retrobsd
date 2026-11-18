/*
 * virtio block disk driver (D36).
 */
#ifndef _MACHINE_VD_H_
#define _MACHINE_VD_H_

#ifdef KERNEL
struct buf;

int vdopen(dev_t dev, int flag, int mode);
int vdclose(dev_t dev, int flag, int mode);
void vdstrategy(struct buf *bp);
daddr_t vdsize(dev_t dev);
int vdioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);
int vdintr(int level, int src);
#endif

#endif /* _MACHINE_VD_H_ */
