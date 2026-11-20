#ifndef _SETJMP_H
#define _SETJMP_H
#ifdef __mc68000__
/*
 * m68k (D42), in longs; see src/libc/m68k/gen/_setjmp.S:
 *	0-5   - d2-d7
 *	6-10  - a2-a6
 *	11    - sp at the call
 *	12    - return address
 *	13    - signal mask saved
 *	14    - signal mask
 *	15-32 - fp2-fp7, 12 bytes each
 */
typedef int jmp_buf [33];
#else
/*
 * Total 12 registers for MIPS architecture:
 *	0  - $s0
 *	1  - $s1
 *	2  - $s2
 *	3  - $s3
 *	4  - $s4
 *	5  - $s5
 *	6  - $s6
 *	7  - $s7
 *	8  - $s8
 *	9  - $ra - return address
 *	10 - $gp - global data pointer
 *	11 - $sp - stack pointer
 *      12 - signal mask saved
 *      13 - signal mask
 */
typedef int jmp_buf [14];
#endif
typedef jmp_buf sigjmp_buf;

/*
 * Save and restore only CPU state.
 * Signal mask is not saved.
 */
int _setjmp (jmp_buf env);
void _longjmp (jmp_buf env, int val);

/*
 * Save and restore CPU state and signal mask.
 */
int setjmp (jmp_buf env);
void longjmp (jmp_buf env, int val);

/*
 * Save and restore CPU state and optionally a signal mask.
 * Signal mask is saved only when savesigs is nonzero.
 */
int sigsetjmp (sigjmp_buf env, int savesigs);
void siglongjmp (sigjmp_buf env, int val);
#endif
