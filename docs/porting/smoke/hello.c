#define TTY ((volatile unsigned int *)0xff008000)

void start(void)
{
    const char *s = "hello m68030\n";
    while (*s)
        *TTY = (unsigned char)*s++;
    for (;;)
        ;
}
