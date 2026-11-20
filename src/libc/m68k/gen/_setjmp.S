/*
 * _setjmp, _longjmp: save and restore the registers a C function must
 * preserve, without the signal mask.  The jmp_buf layout (D42), in longs:
 *
 *	0-5	d2-d7
 *	6-10	a2-a6
 *	11	sp at the call, pointing at the return address
 *	12	return address
 *	13	signal mask saved (setjmp) or not (_setjmp)
 *	14	signal mask
 *	15-32	fp2-fp7, 12 bytes each
 */
#include <machine/asm.h>

ENTRY(_setjmp)
        movl    4(%sp),%a0
        clrl    52(%a0)                 | no signal mask
        clrl    56(%a0)
        movml   %d2-%d7/%a2-%a6,(%a0)
        movl    %sp,44(%a0)
        movl    (%sp),48(%a0)
        fmovemx %fp2-%fp7,60(%a0)
        moveq   #0,%d0
        rts
END(_setjmp)

/*
 * The return address goes back into the slot below the saved sp, since
 * the caller may have used it since setjmp returned; a value of 0 is
 * returned as 1.
 */
ENTRY(_longjmp)
        movl    4(%sp),%a0
        movl    8(%sp),%d0
        jne     1f
        moveq   #1,%d0
1:      movml   (%a0),%d2-%d7/%a2-%a6
        fmovemx 60(%a0),%fp2-%fp7
        movl    44(%a0),%sp
        movl    48(%a0),(%sp)
        rts
END(_longjmp)
