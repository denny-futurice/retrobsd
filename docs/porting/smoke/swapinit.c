/*
 * /sbin/init for the swap test.  Only one user process fits in memory, so
 * when a parent and a child take turns over two pipes, every turn swaps
 * the sleeping one out and the other in (sched(), D38).
 *
 * Each process fills an area of its data, its bss and its stack with its
 * own pattern, then checks all three after every turn.  The child exits
 * with 42; the parent checks that with wait4(), prints "swap ok" and
 * halts.  A wrong pattern or a failed system call prints a message and
 * exits, which panics the kernel with "init died".
 *
 * The patterns are arithmetic byte sequences with a different start and
 * step per area, so run.sh can find each one on the swap partition.
 */
#define SYS_exit        1
#define SYS_fork        2
#define SYS_read        3
#define SYS_write       4
#define SYS_open        5
#define SYS_close       6
#define SYS_wait4       7
#define SYS_pipe        42
#define SYS_reboot      55
#define O_RDWR          2
#define RB_HALT         0x008
#define ROUNDS          8

/* Sizes that are not whole sectors, so swap moves partial sectors. */
#define DATASIZE        777
#define BSSSIZE         3001
#define STACKSIZE       1000

long sys(int n, ...);
long rv1;                       /* d1 of the last system call (pipe) */

/*
 * _start: the kernel enters with sp at argc (D30).  sys(): pop the return
 * address, so that the number is in the return address slot and the
 * arguments are above it (D23); return -errno on an error.
 */
__asm__(
    "   .text\n"
    "   .globl  _start\n"
    "_start:\n"
    "   jsr     main\n"
    "   .globl  sys\n"
    "sys:\n"
    "   move.l  (%sp)+, %a0\n"
    "   move.l  (%sp), %d0\n"
    "   trap    #0\n"
    "   move.l  %d1, rv1\n"
    "   bcc.s   1f\n"
    "   neg.l   %d0\n"
    "1: jmp     (%a0)\n");

char data[DATASIZE] = { 1 };    /* initialized: in .data */
char bss[BSSSIZE];
int console;

static void
put(const char *s)
{
    int n = 0;

    while (s[n])
        n++;
    sys(SYS_write, console, s, n);
}

static void
fail(const char *who, const char *what, int code)
{
    put("swap: ");
    put(who);
    put(what);
    sys(SYS_exit, code);
}

/* Area a of process p (0 parent, 1 child): start and step. */
static unsigned char
first(int p, int a)
{
    return 0x10 + 0x40 * p + 0x10 * a;
}

static unsigned char
step(int p, int a)
{
    return 3 + 2 * (3 * p + a);
}

static void
fill(char *b, int n, int p, int a)
{
    unsigned char v = first(p, a), s = step(p, a);
    int i;

    for (i = 0; i < n; i++, v += s)
        b[i] = v;
}

static int
good(const char *b, int n, int p, int a)
{
    unsigned char v = first(p, a), s = step(p, a);
    int i;

    for (i = 0; i < n; i++, v += s)
        if ((unsigned char) b[i] != v)
            return 0;
    return 1;
}

static void
check(const char *stack, int p, const char *who)
{
    if (!good(data, DATASIZE, p, 0))
        fail(who, "data changed\n", 11);
    if (!good(bss, BSSSIZE, p, 1))
        fail(who, "bss changed\n", 12);
    if (!good(stack, STACKSIZE, p, 2))
        fail(who, "stack changed\n", 13);
}

int
main(void)
{
    char stack[STACKSIZE];
    int down[2], up[2], status, r, p;
    long pid;
    char c;
    const char *who;

    console = sys(SYS_open, "/dev/console", O_RDWR);
    if (console < 0)
        sys(SYS_exit, 1);
    if ((down[0] = sys(SYS_pipe)) < 0)
        fail("", "pipe failed\n", 2);
    down[1] = rv1;
    if ((up[0] = sys(SYS_pipe)) < 0)
        fail("", "pipe failed\n", 2);
    up[1] = rv1;
    if ((pid = sys(SYS_fork)) < 0)
        fail("", "fork failed\n", 3);
    p = pid == 0;
    who = p ? "child: " : "parent: ";
    fill(data, DATASIZE, p, 0);
    fill(bss, BSSSIZE, p, 1);
    fill(stack, STACKSIZE, p, 2);

    for (r = 0; r < ROUNDS; r++) {
        if (p) {
            if (sys(SYS_read, down[0], &c, 1) != 1)
                fail(who, "read failed\n", 4);
            check(stack, p, who);
            if (sys(SYS_write, up[1], &c, 1) != 1)
                fail(who, "write failed\n", 5);
        } else {
            if (sys(SYS_write, down[1], &c, 1) != 1)
                fail(who, "write failed\n", 5);
            if (sys(SYS_read, up[0], &c, 1) != 1)
                fail(who, "read failed\n", 4);
            check(stack, p, who);
        }
    }
    if (p)
        sys(SYS_exit, 42);

    if (sys(SYS_wait4, pid, &status, 0, 0) != pid)
        fail(who, "wait4 failed\n", 6);
    if (status != 42 << 8)
        fail(who, "child status wrong\n", 7);
    check(stack, p, who);
    /* Pipe inodes live on the root; close them or fsutil finds them. */
    sys(SYS_close, down[0]);
    sys(SYS_close, down[1]);
    sys(SYS_close, up[0]);
    sys(SYS_close, up[1]);
    put("swap ok\n");
    sys(SYS_reboot, RB_HALT);
    return 0;
}
