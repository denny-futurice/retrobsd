/*
 * /sbin/init for the rootfs test.  run.sh links it with src/elf32-m68k.ld
 * and converts it with tools/elf2aout, so the boot loads an a.out with
 * text, data and bss.
 *
 * It opens the console, copies /etc/motd to it, and halts with
 * reboot(RB_HALT).  On an error it exits with errno, which panics the
 * kernel with "init died".  System calls follow D23: the arguments above
 * a return address slot, the number in d0, carry set on error.
 */
#define SYS_exit        1
#define SYS_read        3
#define SYS_write       4
#define SYS_open        5
#define SYS_reboot      55
#define O_RDONLY        0
#define O_RDWR          2
#define RB_HALT         0x008
#define BUFSIZE         1000            /* not a block multiple */

        .text
        .globl  _start
_start:
        move.l  #O_RDWR, -(%sp)         /* d6 = open("/dev/console", O_RDWR) */
        pea     console(%pc)
        clr.l   -(%sp)
        moveq   #SYS_open, %d0
        trap    #0
        lea     12(%sp), %sp
        bcs     fail
        move.l  %d0, %d6

        move.l  #O_RDONLY, -(%sp)       /* d7 = open("/etc/motd", O_RDONLY) */
        pea     motd(%pc)
        clr.l   -(%sp)
        moveq   #SYS_open, %d0
        trap    #0
        lea     12(%sp), %sp
        bcs     fail
        move.l  %d0, %d7

copy:
        move.l  #BUFSIZE, -(%sp)        /* d0 = read(d7, buf, BUFSIZE) */
        pea     buf(%pc)
        move.l  %d7, -(%sp)
        clr.l   -(%sp)
        moveq   #SYS_read, %d0
        trap    #0
        lea     16(%sp), %sp
        bcs     fail
        tst.l   %d0
        beq     done

        move.l  %d0, -(%sp)             /* write(d6, buf, d0) */
        pea     buf(%pc)
        move.l  %d6, -(%sp)
        clr.l   -(%sp)
        moveq   #SYS_write, %d0
        trap    #0
        lea     16(%sp), %sp
        bcs     fail
        bra     copy

done:
        move.l  #RB_HALT, -(%sp)        /* reboot(RB_HALT) */
        clr.l   -(%sp)
        moveq   #SYS_reboot, %d0
        trap    #0
fail:
        move.l  %d0, -(%sp)             /* exit(errno) */
        clr.l   -(%sp)
        moveq   #SYS_exit, %d0
        trap    #0

        .data
console:
        .asciz  "/dev/console"
motd:
        .asciz  "/etc/motd"

        .bss
        .balign 4
buf:
        .space  BUFSIZE
