/*
 * pipe(fds): the kernel returns the read end in d0 and the write end in d1.
 */
#include "SYS.h"

ENTRY(pipe)
        SYSTRAP(pipe)
        jcs     cerror
        movl    4(%sp),%a0
        movl    %d0,(%a0)+
        movl    %d1,(%a0)
        moveq   #0,%d0
        rts
END(pipe)
