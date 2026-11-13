/*
 * QEMU m68k "virt" machine: device addresses and bootinfo records.
 * See QEMU hw/m68k/virt.c and include/standard-headers/asm-m68k/bootinfo*.h.
 */
#ifndef _MACHINE_VIRT_H_
#define _MACHINE_VIRT_H_

/*
 * Memory mapped devices.  Goldfish PICs 1 to 6 drive CPU interrupt levels
 * 1 to 6 (autovectors 25 to 30); each has 32 sources, numbered from 1.
 */
#define VIRT_GF_PIC_BASE        0xff000000      /* 6 PICs, 0x1000 apart */
#define VIRT_GF_RTC_BASE        0xff006000      /* 2 RTCs: tick, time of day (D27) */
#define VIRT_GF_TTY_BASE        0xff008000
#define VIRT_CTRL_BASE          0xff009000
#define VIRT_VIRTIO_BASE        0xff010000      /* 128 virtio-mmio slots, 0x200 apart */

#define VIRT_REG(addr)          (*(volatile unsigned *)(addr))

/*
 * Interrupt sources: PIC (= CPU interrupt level) and source number.
 */
#define VIRT_RTC_TICK_LEVEL     6               /* RTC 1, the clock tick (D27) */
#define VIRT_RTC_TICK_SRC       1

/*
 * Goldfish PIC.  IRQ_PENDING reads the pending and enabled sources,
 * source n is bit n-1; ENABLE and DISABLE take such a mask.
 */
#define GF_PIC_ADDR(level, reg) (VIRT_GF_PIC_BASE + ((level) - 1) * 0x1000 + (reg))
#define GF_PIC_BIT(src)         (1u << ((src) - 1))
#define GF_PIC_STATUS           0x00            /* number of pending sources */
#define GF_PIC_IRQ_PENDING      0x04
#define GF_PIC_DISABLE_ALL      0x08            /* also clears pending */
#define GF_PIC_DISABLE          0x0c
#define GF_PIC_ENABLE           0x10

/*
 * Goldfish RTC: nanoseconds since the epoch.  Reading TIME_LOW latches
 * the high word, so read TIME_LOW first.  Writing ALARM_LOW arms the
 * alarm (write ALARM_HIGH first); an alarm already due fires at once.
 * The interrupt stays raised until CLEAR_INTERRUPT is written.
 * RTC 1 is the clock tick, RTC 2 the time of day (D27).
 */
#define VIRT_RTC_TICK           VIRT_GF_RTC_BASE
#define VIRT_RTC_TOD            (VIRT_GF_RTC_BASE + 0x1000)
#define GF_RTC_TIME_LOW         0x00
#define GF_RTC_TIME_HIGH        0x04
#define GF_RTC_ALARM_LOW        0x08
#define GF_RTC_ALARM_HIGH       0x0c
#define GF_RTC_IRQ_ENABLED      0x10
#define GF_RTC_CLEAR_ALARM      0x14
#define GF_RTC_ALARM_STATUS     0x18
#define GF_RTC_CLEAR_INTERRUPT  0x1c

/*
 * Goldfish TTY.  Output is one byte per PUT_CHAR write.  Input is read
 * by DMA: set DATA_PTR and DATA_LEN, then write READ_BUFFER to CMD.
 */
#define GF_TTY_PUT_CHAR         0x00
#define GF_TTY_BYTES_READY      0x04
#define GF_TTY_CMD              0x08
#define GF_TTY_DATA_PTR         0x10
#define GF_TTY_DATA_LEN         0x14
#define GF_TTY_CMD_INT_DISABLE  0
#define GF_TTY_CMD_INT_ENABLE   1
#define GF_TTY_CMD_WRITE_BUFFER 2
#define GF_TTY_CMD_READ_BUFFER  3

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
