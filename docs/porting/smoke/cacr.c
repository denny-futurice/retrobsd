/*
 * Caches off (sys/m68k/startup.S, D34): _start clears and disables both
 * 68030 caches whatever the bootloader left.  QEMU starts with CACR 0, so
 * the first pass turns every cache bit on, as a bootloader might, and runs
 * _start again.  QEMU keeps the write-only clear bits (CI, CD), which read
 * as 0 on a real 68030, so the second pass can also check they were set.
 */
#define TTY ((volatile unsigned int *)0xff008000)
#define CTRL_CMD ((volatile unsigned int *)0xff009004)  /* virt ctrl */

#define CACR_ENABLE 0x3313              /* EI FI IBE ED FD DBE WA */
#define CACR_CLEAR 0x0808               /* CI CD */

int pass = 1;                           /* .data: _start does not clear it */
extern char _start[];

static void puts_(const char *s)
{
    while (*s)
        *TTY = (unsigned char)*s++;
}

/* The vector table points into locore.S, which calls these. */
void trap(void *frame)
{
    puts_("cacr unexpected trap\n");
    for (;;)
        continue;
}

void fpfault_clear(unsigned char *state)
{
}

int main(void)
{
    unsigned cacr;

    if (pass == 1) {
        pass = 2;
        __asm__ volatile ("movec %0, %%cacr\n\tjmp _start"
            : : "d"(CACR_ENABLE));
    }
    __asm__ volatile ("movec %%cacr, %0" : "=d"(cacr));
    if (cacr & CACR_ENABLE)
        puts_("cacr FAIL enabled\n");
    else if ((cacr & CACR_CLEAR) != CACR_CLEAR)
        puts_("cacr FAIL not cleared\n");
    else
        puts_("cacr ok\n");
    *CTRL_CMD = 2;                      /* halt: main must not return here */
    for (;;)
        continue;
}
