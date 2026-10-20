#define TTY ((volatile unsigned int *)0xff008000)

static unsigned long long udiv(unsigned long long a, unsigned long long b)
{
    return a / b;
}

void start(void)
{
    const char *s = "hello m68030 ";
    while (*s)
        *TTY = (unsigned char)*s++;
    *TTY = '0' + (unsigned)udiv(81ULL << 20, 9ULL << 20) % 10;
    *TTY = '\n';
    for (;;)
        ;
}
