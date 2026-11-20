/*
 * /sbin/init for the rootfs test: a hand-built a.out (OMAGIC, D37
 * big-endian header) whose header is the first 32 bytes of this file, so
 * no elf2aout is needed.  run.sh links it with text at USER_DATA_START - 32
 * and copies it out as a raw binary.
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
header:
        .long   0407                    /* a_midmag: OMAGIC, MID_ZERO */
        .long   0                       /* a_text */
        .long   end - _start            /* a_data */
        .long   BUFSIZE                 /* a_bss: buf */
        .long   0, 0, 0                 /* a_reltext, a_reldata, a_syms */
        .long   _start                  /* a_entry */

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

console:
        .asciz  "/dev/console"
motd:
        .asciz  "/etc/motd"
        .balign 4
end:
buf:                                    /* bss, past the loaded image */
