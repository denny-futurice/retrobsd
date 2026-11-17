/*
 * Placeholders for kernel symbols that M3 implements (decision D20), so the
 * M2 kernel links.  M3 removes each entry as the real code arrives, and
 * then this file; only icode is left (task 8).
 */
/*
 * User code for process 1, which execs /sbin/init: startup.S on PIC32.
 * Empty here; process 1 is not started without a root filesystem.
 */
__asm__ (
"       .section .rodata\n"
"       .globl  icode, initflags, icodeend\n"
"icode:\n"
"initflags:\n"
"       .byte   0\n"
"icodeend:\n"
"       .previous\n");
