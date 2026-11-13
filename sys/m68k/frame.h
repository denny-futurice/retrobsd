/*
 * Trap frame for the 68030 (D22).
 *
 * Trap entry (trap.S) leaves the hardware exception frame in place and
 * pushes the registers below it, so the frame is one struct in memory:
 * the saved registers, then the hardware frame starting at tf_sr.  The
 * format-dependent part of the hardware frame follows tf_vector; layouts
 * are from the MC68030 User's Manual, section 8.4, with field names after
 * NetBSD's m68k/include/frame.h.
 */
#ifndef _MACHINE_FRAME_H_
#define _MACHINE_FRAME_H_

/* Format 2: CHK, TRAPV, trace, divide by zero, FPU post-instruction. */
struct fmt2 {
    u_int   f_iaddr;                /* address of the faulting instruction */
};

/* Format 9: coprocessor mid-instruction. */
struct fmt9 {
    u_int   f_iaddr;                /* instruction address */
    u_short f_iregs[4];             /* internal registers */
};

/* Format A: short bus cycle fault. */
struct fmtA {
    u_short f_ir0;
    u_short f_ssw;                  /* special status word */
    u_short f_ipsc;                 /* instruction pipe stage C */
    u_short f_ipsb;                 /* instruction pipe stage B */
    u_int   f_dcfa;                 /* data cycle fault address */
    u_short f_ir1, f_ir2;
    u_int   f_dob;                  /* data output buffer */
    u_short f_ir3, f_ir4;
};

/* Format B: long bus cycle fault. */
struct fmtB {
    u_short f_ir0;
    u_short f_ssw;                  /* special status word */
    u_short f_ipsc;                 /* instruction pipe stage C */
    u_short f_ipsb;                 /* instruction pipe stage B */
    u_int   f_dcfa;                 /* data cycle fault address */
    u_short f_ir1, f_ir2;
    u_int   f_dob;                  /* data output buffer */
    u_short f_ir3, f_ir4;
    u_short f_ir5, f_ir6;
    u_int   f_sba;                  /* stage B address */
    u_short f_ir7, f_ir8;
    u_int   f_dib;                  /* data input buffer */
    u_short f_iregs[22];
};

struct trapframe {
    int     tf_regs[16];            /* d0-d7, a0-a7; a7 is the user sp */
    short   tf_pad;
    short   tf_stackadj;            /* bytes the exit path drops (D22) */
    u_short tf_sr;                  /* hardware frame starts here */
    u_int   tf_pc;                  /* 2-byte aligned (D17), no padding */
    u_short tf_format:4,            /* frame format */
            tf_vector:12;           /* vector offset (vector number * 4) */
    union {
        struct fmt2 F_fmt2;
        struct fmt9 F_fmt9;
        struct fmtA F_fmtA;
        struct fmtB F_fmtB;
    } F_u;
};

#define tf_d0           tf_regs[0]
#define tf_d1           tf_regs[1]
#define tf_sp           tf_regs[15]     /* user stack pointer */

/* Hardware frame sizes in bytes, by format, from tf_sr on. */
#define FMT0SIZE        8
#define FMT1SIZE        8
#define FMT2SIZE        12
#define FMT9SIZE        20
#define FMTASIZE        32
#define FMTBSIZE        92

/* Special status word bits, formats A and B. */
#define SSW_FC          0x8000          /* fault on stage C */
#define SSW_FB          0x4000          /* fault on stage B */
#define SSW_RC          0x2000          /* rerun stage C */
#define SSW_RB          0x1000          /* rerun stage B */
#define SSW_DF          0x0100          /* data fault, rerun data cycle */
#define SSW_RM          0x0080          /* read-modify-write cycle */
#define SSW_RW          0x0040          /* read (1) or write (0) */
#define SSW_SIZE        0x0030          /* data cycle size */
#define SSW_FCMASK      0x0007          /* data cycle function code */

#endif /* _MACHINE_FRAME_H_ */
