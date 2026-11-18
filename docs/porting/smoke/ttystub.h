/*
 * The tty.c routines sys/m68k/gftty.c calls, for the receive interrupt
 * tests.  ttyinput() records each byte and the lowest interrupt level
 * it ran at, and calls first_input, if set, for the first byte.  Include
 * in one file per test.
 */
#include <sys/tty.h>
#include <machine/virt.h>
#include <machine/gftty.h>

#define INPUT   300                     /* bytes run.sh feeds: 'a' + i % 26 */

char inbuf[INPUT];
volatile int nin;
int inipl = -1;
static void (*first_input)(void);

void
ttyinput(int c, struct tty *tp)
{
    int sr;

    __asm__ volatile ("move.w %%sr, %0" : "=d" (sr));
    if (inipl < 0 || (sr & PSL_IPL) < inipl)
        inipl = sr & PSL_IPL;           /* lowest level seen */
    if (nin == 0 && first_input)
        first_input();
    if (nin < INPUT)
        inbuf[nin] = c;
    nin++;
}

/* Every byte arrived, in order. */
static __attribute__((unused)) int
input_ok(void)
{
    int i;

    for (i = 0; i < INPUT; i++)
        if (inbuf[i] != 'a' + i % 26)
            return 0;
    return nin == INPUT;
}

static void
open_console(void)
{
    gfttyttys[0].t_addr = (caddr_t) VIRT_GF_TTY_BASE;
    gfttyopen(makedev(UART_MAJOR, 0), 0, 0);
}

static u_int
rtcnow(void)
{
    return VIRT_REG(VIRT_RTC_TICK + GF_RTC_TIME_LOW);
}

int ttyopen(dev_t dev, struct tty *tp) { return 0; }
void ttyclose(struct tty *tp) { }
void ttychars(struct tty *tp) { }
void ttywflush(struct tty *tp) { }
void ttyowake(struct tty *tp) { }
int ttread(struct tty *tp, struct uio *uio, int flag) { return 0; }
int ttwrite(struct tty *tp, struct uio *uio, int flag) { return 0; }
int ttyselect(struct tty *tp, int rw) { return 0; }
int ttioctl(struct tty *tp, u_int com, caddr_t data, int flag) { return 0; }
int getc(struct clist *p) { return 0; }
