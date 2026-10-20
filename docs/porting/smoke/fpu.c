#define TTY ((volatile unsigned int *)0xff008000)

/* volatile operands force real FPU instructions instead of constant folding */
static volatile double a = 1.5, b = 2.25;

void start(void)
{
    const char *s = "fpu ok\n";
    if ((int)((a * b + a) * 1000) == 4875)
        while (*s)
            *TTY = (unsigned char)*s++;
    else
        *TTY = 'X';
    for (;;)
        ;
}
