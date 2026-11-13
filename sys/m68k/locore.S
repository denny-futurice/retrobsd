/*
 * Exception, trap and interrupt entry for the 68030 (D22).
 *
 * Every vector except the FPU exceptions goes to trapentry.  The entry
 * pushes the registers below the hardware frame, which makes a struct
 * trapframe (sys/m68k/frame.h), and calls trap(), which finds the cause
 * in tf_vector.  The offsets below are checked against frame.h in trap.c.
 */
TF_SP           = 60                    /* tf_regs[15], the user sp */
TF_STACKADJ     = 66
TF_SR           = 68
TF_FMT0END      = TF_SR + 8             /* end of a format 0 frame */

        .text

        .globl  trapentry
        .type   trapentry, @function
trapentry:
        clr.l   -(%sp)                  /* tf_pad, tf_stackadj */
        movem.l %d0-%d7/%a0-%a7, -(%sp)
        move.l  %usp, %a0
        move.l  %a0, TF_SP(%sp)
calltrap:
        move.l  %sp, -(%sp)
        jsr     trap
        addq.l  #4, %sp

/*
 * Return through the trap frame.  trap() may have changed the registers,
 * the user sp, SR and PC.  A non-zero tf_stackadj means trap() turned the
 * hardware frame into a shorter one (trimframe() in trap.c): SR, PC and
 * the format/vector word move up by tf_stackadj bytes, over the dropped
 * part, and the kernel sp follows them.  After NetBSD's faultstkadj.
 */
        .globl  trapret
trapret:
        move.l  TF_SP(%sp), %a0
        move.l  %a0, %usp
        move.w  TF_STACKADJ(%sp), %d0
        bne.s   1f
        movem.l (%sp)+, %d0-%d7/%a0-%a6
        addq.l  #8, %sp                 /* tf_sp, tf_pad, tf_stackadj */
        rte

1:      lea     TF_FMT0END(%sp), %a1    /* source: end of SR, PC, fmt/vec */
        move.l  %a1, %a0
        adda.w  %d0, %a0                /* destination */
        move.l  -(%a1), -(%a0)
        move.l  -(%a1), -(%a0)
        move.l  %a0, TF_SP(%sp)         /* new kernel sp, at the moved SR */
        movem.l (%sp)+, %d0-%d7/%a0-%a6
        move.l  (%sp), %sp
        rte
        .size   trapentry, . - trapentry

/*
 * FPU exceptions, vectors 48-54 (D12).  The 68881/2 holds the exception
 * pending until the handler clears it in the state frame, or frestore
 * would raise it again: fsave, fpfault_clear(), frestore, then trap().
 * After NetBSD's fpfault.  QEMU raises no FPU exceptions, and its 68030
 * treats fsave and frestore as illegal instructions (D25).
 */
        .globl  fpfault
        .type   fpfault, @function
fpfault:
        clr.l   -(%sp)
        movem.l %d0-%d7/%a0-%a7, -(%sp)
        move.l  %usp, %a0
        move.l  %a0, TF_SP(%sp)
        lea     fpfault_state, %a0
        fsave   (%a0)
        move.l  %a0, -(%sp)
        jsr     fpfault_clear
        move.l  (%sp)+, %a0
        frestore (%a0)
        bra.s   calltrap
        .size   fpfault, . - fpfault

/*
 * State frame for fpfault.  The largest is the 68882 busy frame: 4 bytes
 * of header and 0xd4 bytes of state.
 */
        .bss
        .balign 4
        .globl  fpfault_state
fpfault_state:
        .space  256
