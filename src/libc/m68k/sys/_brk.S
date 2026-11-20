/*
 * _brk(addr): the sbrk system call sets the break to addr and returns it.
 */
#include "SYS.h"

ENTRY(_brk)
        SYSTRAP(sbrk)
        jcs     cerror
        rts
END(_brk)
