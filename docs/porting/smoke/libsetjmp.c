/*
 * setjmp and longjmp (D42): the jmp_buf layout of <setjmp.h>, return
 * values, the callee-saved registers d2-d7, a2-a6 and fp2-fp7 restored
 * after a longjmp from a nested call that changed them, and the signal
 * mask restored by longjmp and siglongjmp after sigsetjmp(env, 1) but not
 * by _longjmp or siglongjmp after sigsetjmp(env, 0).
 */
#include "libtest.h"
#include <setjmp.h>
#include <signal.h>
#include <string.h>

static jmp_buf env;
static sigjmp_buf senv;

/*
 * regjmp(env, jump, sj): set the callee-saved registers to known values,
 * sj(env) (setjmp or _setjmp), and on the first return change them all and
 * call jump(env), which must not return.  After the longjmp, return the
 * number of registers that do not hold the known values (0 if all do),
 * or -1 if setjmp returned something other than 0 then 3.
 */
int regjmp(jmp_buf, void (*)(jmp_buf), int (*)(jmp_buf));
asm(
"	.text\n"
"	.globl	regjmp\n"
"regjmp:\n"
"	moveml	%d2-%d7/%a2-%a6,-(%sp)\n"
"	fmovemx	%fp2-%fp7,-(%sp)\n"		/* 44 + 72 bytes */
"	moveq	#2,%d2\n"
"	moveq	#3,%d3\n"
"	moveq	#4,%d4\n"
"	moveq	#5,%d5\n"
"	moveq	#6,%d6\n"
"	moveq	#7,%d7\n"
"	movel	#0x1002,%a2\n"
"	movel	#0x1003,%a3\n"
"	movel	#0x1004,%a4\n"
"	movel	#0x1005,%a5\n"
"	movel	#0x1006,%a6\n"
"	fmovel	#2,%fp2\n"
"	fmovel	#3,%fp3\n"
"	fmovel	#4,%fp4\n"
"	fmovel	#5,%fp5\n"
"	fmovel	#6,%fp6\n"
"	fmovel	#7,%fp7\n"
"	movel	120(%sp),-(%sp)\n"		/* env */
"	movel	132(%sp),%a0\n"			/* sj */
"	jsr	(%a0)\n"
"	addql	#4,%sp\n"
"	tstl	%d0\n"
"	jne	1f\n"
"	moveq	#-1,%d2\n"
"	moveq	#-1,%d3\n"
"	moveq	#-1,%d4\n"
"	moveq	#-1,%d5\n"
"	moveq	#-1,%d6\n"
"	moveq	#-1,%d7\n"
"	movel	%d2,%a2\n"
"	movel	%d2,%a3\n"
"	movel	%d2,%a4\n"
"	movel	%d2,%a5\n"
"	movel	%d2,%a6\n"
"	fmovel	%d2,%fp2\n"
"	fmovel	%d2,%fp3\n"
"	fmovel	%d2,%fp4\n"
"	fmovel	%d2,%fp5\n"
"	fmovel	%d2,%fp6\n"
"	fmovel	%d2,%fp7\n"
"	movel	120(%sp),-(%sp)\n"		/* env */
"	movel	128(%sp),%a0\n"			/* jump */
"	jsr	(%a0)\n"
"	moveq	#-1,%d0\n"			/* jump returned */
"	jra	9f\n"
"1:	moveq	#3,%d1\n"
"	cmpl	%d1,%d0\n"
"	jeq	2f\n"
"	moveq	#-1,%d0\n"
"	jra	9f\n"
"2:	moveq	#0,%d0\n"
"	moveq	#2,%d1\n"
"	cmpl	%d1,%d2; jeq 3f; addql #1,%d0; 3:\n"
"	moveq	#3,%d1\n"
"	cmpl	%d1,%d3; jeq 3f; addql #1,%d0; 3:\n"
"	moveq	#4,%d1\n"
"	cmpl	%d1,%d4; jeq 3f; addql #1,%d0; 3:\n"
"	moveq	#5,%d1\n"
"	cmpl	%d1,%d5; jeq 3f; addql #1,%d0; 3:\n"
"	moveq	#6,%d1\n"
"	cmpl	%d1,%d6; jeq 3f; addql #1,%d0; 3:\n"
"	moveq	#7,%d1\n"
"	cmpl	%d1,%d7; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1002,%a2; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1003,%a3; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1004,%a4; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1005,%a5; jeq 3f; addql #1,%d0; 3:\n"
"	cmpal	#0x1006,%a6; jeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#2,%fp2; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#3,%fp3; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#4,%fp4; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#5,%fp5; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#6,%fp6; fjeq 3f; addql #1,%d0; 3:\n"
"	fcmpl	#7,%fp7; fjeq 3f; addql #1,%d0; 3:\n"
"9:	fmovemx	(%sp)+,%fp2-%fp7\n"
"	moveml	(%sp)+,%d2-%d7/%a2-%a6\n"
"	rts\n");

/*
 * Called with stack in use below regjmp's frame, then longjmp(env, 3) or
 * _longjmp(env, 3).
 */
static void
jump(jmp_buf e)
{
    volatile char pad[256];

    pad[0] = 1;
    longjmp(e, 3 + pad[0] - 1);
}

static void
_jump(jmp_buf e)
{
    volatile char pad[256];

    pad[0] = 1;
    _longjmp(e, 3 + pad[0] - 1);
}

static void
deep(int n)
{
    volatile char pad[64];

    pad[0] = n;
    if (n <= 0)
        longjmp(env, 0);
    else
        deep(pad[0] - 1);
}

static sigset_t
mask(void)
{
    sigset_t m;

    sigprocmask(SIG_BLOCK, 0, &m);
    return m;
}

static void
setmask(sigset_t m)
{
    sigprocmask(SIG_SETMASK, &m, 0);
}

int
main(void)
{
    volatile int count = 0;
    sigset_t usr1 = sigmask(SIGUSR1), usr2 = sigmask(SIGUSR2);
    int r;

    console();
    check(sizeof(jmp_buf) == 33 * 4);
    memset(env, 0, sizeof env);         /* nothing left from a previous run */
    check(regjmp(env, jump, setjmp) == 0);
    memset(env, 0, sizeof env);
    check(regjmp(env, _jump, _setjmp) == 0);

    r = setjmp(env);
    if (r == 0) {
        count++;
        deep(10);
    }
    check(r == 1 && count == 1);

    setmask(usr1);
    if (setjmp(env) == 0) {
        setmask(usr2);
        longjmp(env, 1);
    }
    check(mask() == usr1);

    setmask(usr1);
    if (_setjmp(env) == 0) {
        setmask(usr2);
        _longjmp(env, 1);
    }
    check(mask() == usr2);

    setmask(usr1);
    if ((r = sigsetjmp(senv, 1)) == 0) {
        setmask(usr2);
        siglongjmp(senv, 5);
    }
    check(r == 5 && mask() == usr1);

    setmask(usr1);
    if ((r = sigsetjmp(senv, 0)) == 0) {
        setmask(usr2);
        siglongjmp(senv, 6);
    }
    check(r == 6 && mask() == usr2);

    finish("libsetjmp");
    return 0;
}
