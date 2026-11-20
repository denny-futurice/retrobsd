/*
 * setjmp, longjmp: as _setjmp and _longjmp (see _setjmp.S for the jmp_buf
 * layout), and the signal mask is saved and restored as well.
 */
#include <machine/asm.h>

#define SIG_BLOCK       1
#define SIG_SETMASK     3

ENTRY(setjmp)
        movl    4(%sp),%a0
        pea     56(%a0)                 | sigprocmask(SIG_BLOCK, 0, &mask)
        clrl    -(%sp)
        pea     SIG_BLOCK
        jbsr    sigprocmask
        lea     12(%sp),%sp
        movl    4(%sp),%a0
        moveq   #1,%d0
        movl    %d0,52(%a0)             | signal mask saved
        movml   %d2-%d7/%a2-%a6,(%a0)
        movl    %sp,44(%a0)
        movl    (%sp),48(%a0)
        fmovemx %fp2-%fp7,60(%a0)
        moveq   #0,%d0
        rts
END(setjmp)

ENTRY(longjmp)
        movl    4(%sp),%a0
        clrl    -(%sp)                  | sigprocmask(SIG_SETMASK, &mask, 0)
        pea     56(%a0)
        pea     SIG_SETMASK
        jbsr    sigprocmask
        lea     12(%sp),%sp
        jra     _longjmp
END(longjmp)
