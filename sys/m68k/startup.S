/*
 * Kernel startup for m68k (68030) on QEMU virt.
 *
 * QEMU loads the ELF image into RAM and jumps to _start in supervisor mode;
 * there is no boot ROM.  The memory map is in sys/m68k/virt/kernel.ld (D16).
 */

/*
 * Exception vector table, at address 0 (kernel.ld).  Vectors 0 and 1 are
 * the reset SSP and PC.  The FPU exceptions go to fpfault, every other
 * vector to trapentry; both are in locore.S, and trap() sorts them out.
 */
        .section .vectors,"aw"
        .globl  vectors
vectors:
        .long   u_end                   /* 0: reset initial SSP */
        .long   _start                  /* 1: reset initial PC */
        .rept   46
        .long   trapentry               /* 2-47 */
        .endr
        .rept   7
        .long   fpfault                 /* 48-54: FPU exceptions */
        .endr
        .rept   201
        .long   trapentry               /* 55-255 */
        .endr

        .text

/*
 * Entry point.  QEMU's bootinfo records sit just after _end, so nothing
 * here writes above _end.
 */
        .globl  _start
        .type   _start, @function
_start:
        move.w  #0x2700, %sr            /* supervisor, interrupts masked */
        lea     u_end, %sp              /* kernel stack at the top of u */
        lea     vectors, %a0
        movec   %a0, %vbr

        lea     __bss_start, %a0        /* clear BSS */
        lea     __bss_end, %a1
1:      cmp.l   %a1, %a0
        bcc.s   2f
        clr.l   (%a0)+
        bra.s   1b
2:
        jsr     main

        /*
         * main() returns in process 1, which then runs in user mode at
         * USER_DATA_START, as on PIC32.  Entering user mode is M3 work;
         * until then, halt.
         */
3:      stop    #0x2700
        bra.s   3b
        .size   _start, . - _start
