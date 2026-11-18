/*
 * virtio 1.0 over MMIO (the virtio specification, section 4.2.2), and the
 * block device.  Registers, rings and the configuration space are
 * little-endian (D36).
 */
#ifndef _MACHINE_VIRTIO_H_
#define _MACHINE_VIRTIO_H_

#define VIRTIO_MMIO_MAGIC_VALUE         0x000   /* "virt" */
#define VIRTIO_MMIO_VERSION             0x004   /* 2: virtio 1.0 */
#define VIRTIO_MMIO_DEVICE_ID           0x008
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL           0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX       0x034
#define VIRTIO_MMIO_QUEUE_NUM           0x038
#define VIRTIO_MMIO_QUEUE_READY         0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x060
#define VIRTIO_MMIO_INTERRUPT_ACK       0x064
#define VIRTIO_MMIO_STATUS              0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW    0x090
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH   0x094
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW    0x0a0
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH   0x0a4
#define VIRTIO_MMIO_CONFIG              0x100

#define VIRTIO_MMIO_MAGIC       0x74726976
#define VIRTIO_ID_BLOCK         2

/* Device status */
#define VIRTIO_STATUS_ACKNOWLEDGE       0x01
#define VIRTIO_STATUS_DRIVER            0x02
#define VIRTIO_STATUS_DRIVER_OK         0x04
#define VIRTIO_STATUS_FEATURES_OK       0x08

/* Feature bits 32-63 are selected with FEATURES_SEL 1 */
#define VIRTIO_F_VERSION_1_HI   0x00000001      /* feature 32 */
#define VIRTIO_BLK_F_RO         0x00000020      /* feature 5 */

/* Split virtqueue */
struct vring_desc {
    u_int   addr_lo, addr_hi;
    u_int   len;
    u_short flags;
    u_short next;
};
#define VRING_DESC_F_NEXT       1
#define VRING_DESC_F_WRITE      2               /* device writes the buffer */

/* Block requests; the configuration space starts with the capacity */
struct virtio_blk_req {
    u_int   type;
    u_int   reserved;
    u_int   sector_lo, sector_hi;
};
#define VIRTIO_BLK_T_IN         0
#define VIRTIO_BLK_T_OUT        1
#define VIRTIO_BLK_S_OK         0
#define VIRTIO_BLK_SECTOR       512

/* Little-endian <-> big-endian */
static inline u_int
le32(u_int x)
{
    return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

static inline u_short
le16(u_short x)
{
    return (u_short) ((x >> 8) | (x << 8));
}

/*
 * Registers allow 32-bit accesses only.  GCC narrows a masked volatile
 * read to a byte access (btst #0,addr), so go through move.l.
 */
static inline u_int
virtio_read(u_int addr)
{
    u_int v;

    asm volatile ("movel %1@,%0" : "=d" (v) : "a" (addr) : "memory");
    return le32(v);
}

static inline void
virtio_write(u_int addr, u_int v)
{
    asm volatile ("movel %0,%1@" : : "d" (le32(v)), "a" (addr) : "memory");
}

#endif /* _MACHINE_VIRTIO_H_ */
