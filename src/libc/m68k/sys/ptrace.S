/*
 * ptrace(request, pid, addr, data): -1 is a valid result of a read, so
 * errno is cleared first and the caller tells them apart by it.
 */
#include "SYS.h"

ENTRY(ptrace)
        clrl    errno
        SYSTRAP(ptrace)
        jcs     cerror
        rts
END(ptrace)
