/*
 * sigaction(sig, act, oact): the system call takes the address of the
 * signal trampoline as a 4th argument (D23), so the stub pushes the three
 * arguments again with the trampoline above them and a dummy return
 * address below, where the kernel expects the stub's own.
 */
#include "SYS.h"

ENTRY(sigaction)
        pea     sigtramp                | 4th argument: the trampoline
        movl    16(%sp),-(%sp)          | oact
        movl    16(%sp),-(%sp)          | act
        movl    16(%sp),-(%sp)          | sig
        clrl    -(%sp)                  | dummy return address
        SYSTRAP(sigaction)
        lea     20(%sp),%sp             | keeps the carry
        jcs     cerror
        rts
END(sigaction)

/*
 * Signal trampoline (D31).  The kernel enters the handler with this as its
 * return address; the handler returns here with sp at the signal number,
 * then come code, the scp slot (which a C handler may change) and the
 * sigcontext at sp + 12.  sigreturn(&sigcontext) does not return when it
 * succeeds; if it fails the process exits with the errno as its status.
 */
        .text
        .even
        .type   sigtramp,@function
sigtramp:
        pea     12(%sp)                 | &sigcontext
        clrl    -(%sp)                  | dummy return address
        SYSTRAP(sigreturn)
        movl    %d0,4(%sp)
        SYSTRAP(exit)
        .size   sigtramp,.-sigtramp
