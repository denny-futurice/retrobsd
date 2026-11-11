/*
 * Unexpected exception path (sys/m68k/startup.S): trap #15 goes through
 * the vector table to badvec, which calls badtrap() with the vector number
 * (47), the PC after the trap instruction and the supervisor-mode SR.
 */
#define TTY ((volatile unsigned int *)0xff008000)

extern char after_trap[];

static void puts_(const char *s)
{
    while (*s)
        *TTY = (unsigned char)*s++;
}

void badtrap(int vec, unsigned pc, int sr)
{
    if (vec == 47 && pc == (unsigned)after_trap && (sr & 0x2000))
        puts_("badvec ok\n");
    else
        puts_("badvec FAIL\n");
}

int main(void)
{
    __asm__ volatile ("trap #15\n"
                      ".globl after_trap\n"
                      "after_trap:");
    puts_("badvec FAIL no trap\n");
    return 0;
}
