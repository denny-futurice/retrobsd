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
        move.l  #0x0808, %d0            /* CACR: clear both caches, all off */
        movec   %d0, %cacr              /* whatever the bootloader left (D34) */

        lea     __bss_start, %a0        /* clear BSS */
        lea     __bss_end, %a1
1:      cmp.l   %a1, %a0
        bcc.s   2f
        clr.l   (%a0)+
        bra.s   1b
2:
        jsr     main

        /*
         * main() returns in process 1, which has icode at USER_DATA_START:
         * enter it in user mode, as on PIC32.  The kernel stack starts
         * again at the top of u, the user stack at USER_DATA_END, and no
         * kernel value is left in a register.  SR 0 is user mode, IPL 0.
         */
        move.w  #0x2700, %sr
        lea     u_end, %sp
        lea     USER_DATA_END, %a0
        move.l  %a0, %usp
        clr.w   -(%sp)                  /* format 0 */
        pea     USER_DATA_START         /* PC */
        clr.w   -(%sp)                  /* SR */
        moveq   #0, %d0
        moveq   #0, %d1
        moveq   #0, %d2
        moveq   #0, %d3
        moveq   #0, %d4
        moveq   #0, %d5
        moveq   #0, %d6
        moveq   #0, %d7
        movea.l %d0, %a0
        movea.l %d0, %a1
        movea.l %d0, %a2
        movea.l %d0, %a3
        movea.l %d0, %a4
        movea.l %d0, %a5
        movea.l %d0, %a6
        rte
        .size   _start, . - _start

/*
 * User code for process 1, copied to USER_DATA_START by main(): exec
 * /sbin/init, and exit with the error if that fails.  It is position
 * independent, and builds argv on the user stack.  System calls follow
 * D23: the arguments above a return address slot, the number in d0.
 * main() patches initflags to "-s" for single user mode.
 */
        .globl  icode, initflags, icodeend
icode:
        clr.l   -(%sp)                  /* argv[2] */
        pea     initflags(%pc)          /* argv[1] */
        pea     etcinit+6(%pc)          /* argv[0]: "init" */
        move.l  %sp, %a0
        move.l  %a0, -(%sp)             /* execv(path, argv) */
        pea     etcinit(%pc)
        clr.l   -(%sp)
        moveq   #11, %d0                /* SYS_execv */
        trap    #0
        move.l  %d0, -(%sp)             /* exit(errno) */
        clr.l   -(%sp)
        moveq   #1, %d0                 /* SYS_exit */
        trap    #0
etcinit:
        .asciz  "/sbin/init"
initflags:
        .asciz  "-"                     /* space for options */
        .space  10
        .balign 2
icodeend:
