/*
 * User FPU state of the 68881/2 (D12, D28), in the u area and in the
 * signal context (D31).
 */
#ifndef _MACHINE_FPU_H_
#define _MACHINE_FPU_H_

struct fpstate {
    unsigned char fps_frame[216];   /* fsave frame, 68882 busy is largest */
    unsigned int fps_regs[8 * 3];   /* fp0-fp7, 96-bit extended */
    unsigned int fps_fpcr;          /* control registers */
    unsigned int fps_fpsr;
    unsigned int fps_fpiar;
};

/* fps_frame[1], the frame size after the 4-byte header (MC68881/2 UM 6.4). */
#define FPF_NULL        0x00            /* null frame: the FPU is reset */
#define FPF_IDLE81      0x18
#define FPF_IDLE82      0x38
#define FPF_BUSY81      0xb4
#define FPF_BUSY82      0xd4

#endif /* _MACHINE_FPU_H_ */
