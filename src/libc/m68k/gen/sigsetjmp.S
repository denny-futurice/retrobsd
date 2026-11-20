/*
 * sigsetjmp(env, savesigs), siglongjmp(env, val): setjmp or _setjmp as
 * savesigs asks; siglongjmp restores the mask if sigsetjmp saved it.
 * Both jump on with the caller's stack as it is.
 */
#include <machine/asm.h>

ENTRY(sigsetjmp)
        tstl    8(%sp)
        jne     setjmp
        jra     _setjmp
END(sigsetjmp)

ENTRY(siglongjmp)
        movl    4(%sp),%a0
        tstl    52(%a0)
        jne     longjmp
        jra     _longjmp
END(siglongjmp)
