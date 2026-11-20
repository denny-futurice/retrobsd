/*
 * Input for the elf2aout test, never run.  Linked with src/elf32-m68k.ld it
 * has text with an entry point past its start, rodata of an odd length, a
 * gap up to the 16-byte aligned data, data of an odd length, and bss.
 */
        .text
        .globl  _start
        nop
_start:
        bra.s   _start

        .section .rodata
        .ascii  "rodata"

        .data
        .balign 16
        .ascii  "data ok"

        .bss
        .space  100
