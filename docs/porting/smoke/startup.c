/*
 * Kernel startup (sys/m68k/startup.S): BSS is cleared, the stack is in the
 * u area and VBR points at the vector table.  run.sh fills smoke_bss with a
 * pattern through QEMU's loader device before startup runs.
 */
#define TTY ((volatile unsigned int *)0xff008000)

unsigned smoke_bss[4];
extern char u[], u_end[], vectors[];

static void puts_(const char *s)
{
    while (*s)
        *TTY = (unsigned char)*s++;
}

/* The vector table points into locore.S, which calls these. */
void trap(void *frame)
{
    puts_("startup unexpected trap\n");
    for (;;)
        continue;
}

void fpfault_clear(unsigned char *state)
{
}

int main(void)
{
    unsigned sp, vbr;

    __asm__ volatile ("move.l %%sp, %0" : "=d"(sp));
    __asm__ volatile ("movec %%vbr, %0" : "=d"(vbr));
    if (smoke_bss[0] | smoke_bss[1] | smoke_bss[2] | smoke_bss[3])
        puts_("startup FAIL bss\n");
    else if (sp <= (unsigned)u || sp > (unsigned)u_end)
        puts_("startup FAIL sp\n");
    else if (vbr != (unsigned)vectors)
        puts_("startup FAIL vbr\n");
    else
        puts_("startup ok\n");
    return 0;
}
