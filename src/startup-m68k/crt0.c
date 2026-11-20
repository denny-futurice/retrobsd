/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>
#include <unistd.h>

/*
 * C runtime startoff.  When an a.out is loaded by the kernel, the kernel
 * sets up the stack as follows (D30):
 *
 *	_________________________________
 *	| pointer to argv[0], for ps	| top of the stack
 *	|-------------------------------|
 *	| argument and environment	|
 *	| strings			|
 *	|-------------------------------|
 *	| envp[envc] (NULL)		|
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| envp[0]			|
 *	|-------------------------------|
 *	| argv[argc] (NULL)		|
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| argv[0]			|
 *	|-------------------------------|
 * sp->	| argc				|
 *	---------------------------------
 *
 * _start passes argc, argv and envp to __start as C arguments; __start
 * sets environ and __progname and calls main.
 */
extern int main (int, char **, char **);

char **environ;
const char *__progname = "";

void __start (int, char **, char **);

asm(
"	.text\n"
"	.globl	_start\n"
"	.type	_start,@function\n"
"_start:\n"
"	movl	(%sp),%d0\n"			/* argc */
"	lea	4(%sp),%a0\n"			/* argv */
"	lea	4(%a0,%d0.l*4),%a1\n"		/* envp: past argv[argc] */
"	movl	%a1,-(%sp)\n"
"	movl	%a0,-(%sp)\n"
"	movl	%d0,-(%sp)\n"
"	clrl	-(%sp)\n"			/* no return address */
"	jmp	__start\n"
"	.size	_start,.-_start\n");

void
__start (int argc, char **argv, char **env)
{
	environ = env;
	if (argc > 0 && argv[0] != 0) {
		const char *s;

		__progname = argv[0];
		for (s = __progname; *s != '\0'; s++)
			if (*s == '/')
				__progname = s + 1;
	}
	exit (main (argc, argv, env));
}
