/*
 * m68k system call stubs (D23): trap #0 with the number in d0 and the
 * arguments on the stack above the return address.  On error the kernel
 * sets the carry and puts the errno in d0; cerror stores it and returns -1.
 */
#include <syscall.h>
#include <machine/asm.h>

/* moveq reaches numbers up to 127 only. */
#define SYSTRAP(s) \
        .if SYS_##s < 128; moveq #SYS_##s,%d0; .else; movel #SYS_##s,%d0; .endif; \
        trap #0

#define SYS(s)  ENTRY(s); SYSTRAP(s); jcs cerror; rts; END(s)
