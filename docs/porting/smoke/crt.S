    .text
    .globl _start
_start:
    move.l #0x100000,%sp
    jsr start
