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

/*
 * Context switch, after PIC32's startup.S and NetBSD's m68k locore.
 * Offsets are checked in trap.c.
 */
P_ADDR          = 56                    /* struct proc p_addr */
U_FPSTATE       = 1002                  /* struct user u_fpstate */
FPS_REGS        = 216                   /* struct fpstate fps_regs */
FPS_FPCR        = 312                   /* fps_fpcr, fps_fpsr, fps_fpiar */
LABEL_PC        = 48                    /* label_t val[12] */
USIZE           = 4096

        .text

/*
 * Save the user FPU state in u.u_fpstate from a0, and restore it.  Boards
 * whose CPU model lacks fsave/frestore (QEMU's 68030) set FPU_NO_FSAVE and
 * keep only the registers (D28).  Otherwise a null fsave frame means the
 * FPU was never used, and frestore of it resets the FPU.
 *
 * The control registers are moved one at a time: QEMU's fmovem.l stores
 * and loads them in reverse order (FPIAR at the lowest address), while the
 * 68881/2 puts FPCR first (docs/qemu, target/m68k/translate.c,
 * gen_op_fmove_fcr).
 */
        .macro  FPCTLSAVE
        fmove.l %fpcr, FPS_FPCR(%a0)
        fmove.l %fpsr, FPS_FPCR+4(%a0)
        fmove.l %fpiar, FPS_FPCR+8(%a0)
        .endm

        .macro  FPCTLRESTORE
        fmove.l FPS_FPCR(%a0), %fpcr
        fmove.l FPS_FPCR+4(%a0), %fpsr
        fmove.l FPS_FPCR+8(%a0), %fpiar
        .endm

        .macro  FPSAVE
#ifdef FPU_NO_FSAVE
        fmovem.x %fp0-%fp7, FPS_REGS(%a0)
        FPCTLSAVE
#else
        fsave   (%a0)
        tst.b   (%a0)
        beq.s   1f
        fmovem.x %fp0-%fp7, FPS_REGS(%a0)
        FPCTLSAVE
1:
#endif
        .endm

        .macro  FPRESTORE
#ifdef FPU_NO_FSAVE
        FPCTLRESTORE
        fmovem.x FPS_REGS(%a0), %fp0-%fp7
#else
        tst.b   (%a0)
        beq.s   1f
        FPCTLRESTORE
        fmovem.x FPS_REGS(%a0), %fp0-%fp7
1:      frestore (%a0)
#endif
        .endm

/*
 * void savefpu(void);
 *
 * Copy the live FPU state to u.u_fpstate and leave the FPU as it was.
 * fork() calls it before swapout() copies u for the child.
 */
        .globl  savefpu
        .type   savefpu, @function
savefpu:
        lea     u+U_FPSTATE, %a0
        FPSAVE
        FPRESTORE
        rts
        .size   savefpu, . - savefpu

/*
 * void restorefpu(void);
 *
 * Load u.u_fpstate into the FPU.  exec() zeroes u_fpstate first: that is
 * a null frame, so frestore resets the FPU, or with FPU_NO_FSAVE zero
 * registers (D30).
 */
        .globl  restorefpu
        .type   restorefpu, @function
restorefpu:
        lea     u+U_FPSTATE, %a0
        FPRESTORE
        rts
        .size   restorefpu, . - restorefpu

/*
 * int setjmp(label_t *env);
 *
 * Save d2-d7, a2-a6, the sp and the return pc in env; return 0.
 */
        .globl  setjmp
        .type   setjmp, @function
setjmp:
        move.l  4(%sp), %a0
        movem.l %d2-%d7/%a2-%a7, (%a0)
        move.l  (%sp), LABEL_PC(%a0)
        moveq   #0, %d0
        rts
        .size   setjmp, . - setjmp

/*
 * void longjmp(size_t unew, label_t *env);
 *
 * Return 1 from the setjmp() that saved env.  If unew is not &u, the
 * process in u0 is switched in first: the FPU state is saved in u, the
 * contents of u and u0 are exchanged and the FPU state of the new process
 * is loaded from u.  The kernel stack lives in u, so the exchange runs at
 * IPL 7 without the stack, and env is read only after it, from the new u.
 * Returns at IPL 0, as PIC32 does.
 */
        .globl  longjmp
        .type   longjmp, @function
longjmp:
        move.w  #0x2700, %sr
        move.l  8(%sp), %a1             /* env */
        cmp.l   #u, 4(%sp)
        beq.s   2f

        lea     u+U_FPSTATE, %a0
        FPSAVE
        move.l  u, %a0                  /* u.u_procp->p_addr = &u0 */
        move.l  #u0, P_ADDR(%a0)
        lea     u, %a2
        lea     u0, %a3
        move.w  #USIZE/4-1, %d2
1:      move.l  (%a2), %d0
        move.l  (%a3), (%a2)+
        move.l  %d0, (%a3)+
        dbra    %d2, 1b
        move.l  u, %a0                  /* u.u_procp->p_addr = &u */
        move.l  #u, P_ADDR(%a0)
        lea     u+U_FPSTATE, %a0
        FPRESTORE

2:      movem.l (%a1)+, %d2-%d7/%a2-%a7
        move.l  (%a1), (%sp)            /* pc */
        moveq   #1, %d0
        move.w  #0x2000, %sr
        rts
        .size   longjmp, . - longjmp
