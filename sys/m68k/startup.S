/*
 * Kernel startup for m68k (68030) on QEMU virt.
 *
 * QEMU loads the ELF image into RAM and jumps to _start in supervisor mode;
 * there is no boot ROM.  The memory map is in sys/m68k/virt/kernel.ld (D16).
 */

/*
 * Exception vector table, at address 0 (kernel.ld).  Vectors 0 and 1 are
 * the reset SSP and PC; every other vector goes to badvec until the trap
 * code (M3) installs real handlers.
 */
        .section .vectors,"aw"
        .globl  vectors
vectors:
        .long   u_end                   /* 0: reset initial SSP */
        .long   _start                  /* 1: reset initial PC */
        .rept   254
        .long   badvec                  /* 2-255 */
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

/*
 * Unexpected exception.  The 68030 frame starts with SR (word), PC (long)
 * and the format/vector word.  Calls badtrap(vector number, pc, sr), which
 * reports it; halt if it returns.
 */
        .globl  badvec
        .type   badvec, @function
badvec:
        move.w  #0x2700, %sr
        moveq   #0, %d0
        move.w  (%sp), %d0              /* sr */
        move.l  %d0, -(%sp)
        move.l  6(%sp), -(%sp)          /* pc */
        move.w  14(%sp), %d0            /* format/vector word */
        andi.l  #0x0fff, %d0
        lsr.l   #2, %d0                 /* vector number */
        move.l  %d0, -(%sp)
        jsr     badtrap
1:      stop    #0x2700
        bra.s   1b
        .size   badvec, . - badvec
