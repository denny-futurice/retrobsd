/*
 * sigaction and the signal trampoline (D23, D31): the stub's added
 * trampoline argument, the old action returned, an invalid signal failing
 * through the carry, the handler's arguments and its blocked mask, and on
 * return every register the program had when the signal came, even with
 * the handler's scp argument changed and its scratch registers used.
 */
#include "libtest.h"
#include <signal.h>

static volatile int calls, hsig;
static volatile sigset_t hmask;
static volatile struct sigcontext *hscp;

static void
handler(int sig, int code, struct sigcontext *scp)
{
    volatile double x = 1.5;

    calls++;
    hsig = sig;
    hscp = scp;
    sigprocmask(SIG_BLOCK, 0, (sigset_t *) &hmask);
    x = x * x;                                  /* uses fp0/fp1 */
    *(struct sigcontext *volatile *) &scp = 0;  /* the scp slot */
    asm volatile("moveq #-1,%%d0; movel %%d0,%%d1; movel %%d0,%%a0; movel %%d0,%%a1"
        ::: "d0", "d1", "a0", "a1");
}

/*
 * sigregs(): set d2-d7, a0-a6 and fp0-fp7 to known values, send itself
 * SIGUSR1 with kill(getpid(), SIGUSR1) as a bare trap, and return the
 * number of those registers changed when the trap returns (d0 holds the
 * result of kill, and the kernel may change d1, D23).
 */
int sigregs(void);
asm(
"	.text\n"
"	.globl	sigregs\n"
"sigregs:\n"
"	moveml	%d2-%d7/%a2-%a6,-(%sp)\n"
"	fmovemx	%fp2-%fp7,-(%sp)\n"
"	moveq	#20,%d0\n"			/* SYS_getpid */
"	trap	#0\n"
"	movel	#30,-(%sp)\n"			/* SIGUSR1 */
"	movel	%d0,-(%sp)\n"			/* pid */
"	clrl	-(%sp)\n"			/* return address slot */
"	moveq	#2,%d2\n"
"	moveq	#3,%d3\n"
"	moveq	#4,%d4\n"
"	moveq	#5,%d5\n"
"	moveq	#6,%d6\n"
"	moveq	#7,%d7\n"
"	movel	#0x1000,%a0\n"
"	movel	#0x1001,%a1\n"
"	movel	#0x1002,%a2\n"
"	movel	#0x1003,%a3\n"
"	movel	#0x1004,%a4\n"
"	movel	#0x1005,%a5\n"
"	movel	#0x1006,%a6\n"
"	fmovel	#10,%fp0\n"
"	fmovel	#11,%fp1\n"
"	fmovel	#12,%fp2\n"
"	fmovel	#13,%fp3\n"
"	fmovel	#14,%fp4\n"
"	fmovel	#15,%fp5\n"
"	fmovel	#16,%fp6\n"
"	fmovel	#17,%fp7\n"
"	moveq	#37,%d0\n"			/* SYS_kill */
"	trap	#0\n"
"	lea	12(%sp),%sp\n"
"	moveq	#0,%d0\n"
"	cmpl	#2,%d2; jeq 3f; addql #1,%d0; 3:\n"
"	cmpl	#3,%d3; jeq 3f; addql #1,%d0; 3:\n"
"	cmpl	#4,%d4; jeq 3f; addql #1,%d0; 3:\n"
"	cmpl	#5,%d5; jeq 3f; addql #1,%d0; 3:\n"
"	cmpl	#6,%d6; jeq 3f; addql #1,%d0; 3:\n"
"	cmpl	#7,%d7; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1000,%a0; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1001,%a1; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1002,%a2; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1003,%a3; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1004,%a4; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1005,%a5; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1006,%a6; jeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#10,%fp0; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#11,%fp1; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#12,%fp2; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#13,%fp3; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#14,%fp4; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#15,%fp5; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#16,%fp6; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#17,%fp7; fjeq 3f; addql #1,%d0; 3:\n"
"	fmovemx	(%sp)+,%fp2-%fp7\n"
"	moveml	(%sp)+,%d2-%d7/%a2-%a6\n"
"	rts\n");

int
main(void)
{
    struct sigaction sa, osa;
    sigset_t m;

    console();

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = (void (*)(int)) handler;
    sa.sa_mask = sigmask(SIGUSR2);
    check(sigaction(SIGUSR1, &sa, 0) == 0);
    check(sigregs() == 0);
    check(calls == 1);
    check(hsig == SIGUSR1);
    check(hscp != 0 && hscp->sc_mask == 0);
    check(hmask == (sigmask(SIGUSR1) | sigmask(SIGUSR2)));
    sigprocmask(SIG_BLOCK, 0, &m);
    check(m == 0);

    check(kill(getpid(), SIGUSR1) == 0);
    check(calls == 2);

    sa.sa_handler = SIG_IGN;
    memset(&osa, 0, sizeof osa);
    check(sigaction(SIGUSR1, &sa, &osa) == 0);
    check(osa.sa_handler == (void (*)(int)) handler);
    check(osa.sa_mask == sigmask(SIGUSR2));
    check(kill(getpid(), SIGUSR1) == 0);
    check(calls == 2);

    errno = 0;
    check(sigaction(NSIG, &sa, 0) == -1 && errno == EINVAL);
    check(sigaction(SIGKILL, &sa, 0) == -1 && errno == EINVAL);

    finish("libsignal");
    return 0;
}
