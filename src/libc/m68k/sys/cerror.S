/*
 * cerror: system call error path, branched to (not called) by the stubs
 * with the errno in d0.  Returns -1 to the stub's caller, in a0 as well
 * for calls whose result is a pointer.
 */
#include "SYS.h"

ENTRY(cerror)
        movl    %d0,errno
        moveq   #-1,%d0
        movl    %d0,%a0
        rts
END(cerror)
