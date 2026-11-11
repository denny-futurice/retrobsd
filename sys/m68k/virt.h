/*
 * QEMU m68k "virt" machine: device addresses and bootinfo records.
 * See QEMU hw/m68k/virt.c and include/standard-headers/asm-m68k/bootinfo*.h.
 */
#ifndef _MACHINE_VIRT_H_
#define _MACHINE_VIRT_H_

/*
 * Memory mapped devices.  Each Goldfish PIC serves one CPU interrupt
 * level (PIC n -> IPL n+1) with 32 sources.
 */
#define VIRT_GF_PIC_BASE        0xff000000      /* 6 PICs, 0x1000 apart */
#define VIRT_GF_RTC_BASE        0xff006000      /* 2 RTCs, the second is the timer */
#define VIRT_GF_TTY_BASE        0xff008000
#define VIRT_CTRL_BASE          0xff009000
#define VIRT_VIRTIO_BASE        0xff010000      /* 128 virtio-mmio slots, 0x200 apart */

#define VIRT_REG(addr)          (*(volatile unsigned *)(addr))

/*
 * Goldfish RTC: nanoseconds since the epoch.  Reading TIME_LOW latches
 * the high word, so read TIME_LOW first.
 */
#define GF_RTC_TIME_LOW         0x00
#define GF_RTC_TIME_HIGH        0x04

/*
 * virt-ctrl: system control.  Writing HALT or PANIC makes QEMU exit,
 * RESET restarts the machine.
 */
#define VIRT_CTRL_FEATURES      0x00
#define VIRT_CTRL_CMD           0x04
#define VIRT_CTRL_CMD_NOOP      0
#define VIRT_CTRL_CMD_RESET     1
#define VIRT_CTRL_CMD_HALT      2
#define VIRT_CTRL_CMD_PANIC     3

/*
 * Bootinfo: QEMU places a list of records right after the kernel image
 * (_end, rounded up to an even address), ended by a BI_LAST record.
 * bi_size is the size of the whole record including the header.
 */
struct bi_record {
    unsigned short  bi_tag;
    unsigned short  bi_size;
    unsigned long   bi_data[];
};

#define BI_LAST                 0x0000
#define BI_MACHTYPE             0x0001
#define BI_CPUTYPE              0x0002
#define BI_FPUTYPE              0x0003
#define BI_MMUTYPE              0x0004
#define BI_MEMCHUNK             0x0005          /* data: address, size */

#endif /* _MACHINE_VIRT_H_ */
