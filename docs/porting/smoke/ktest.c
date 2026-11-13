/*
 * Support for the smoke tests that link the kernel's machdep.c: the kernel
 * symbols machdep.c and locore.S need, a console that writes straight to
 * the Goldfish TTY, and a check() helper.  Not a test itself.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kconfig.h>
#include <sys/reboot.h>
#include <machine/virt.h>
#include "ktest.h"

size_t physmem;
int boothowto;
__attribute__((weak)) int noproc;       /* kern_clock.c, if linked */
dev_t rootdev, dumpdev = NODEV;
struct conf_ctlr conf_ctlr_init[1];
struct conf_device conf_device_init[1];

static int failed;

struct fs *
getfs(dev_t dev)
{
    return 0;
}

void
sync()
{
}

void
kputs(const char *s)
{
    while (*s)
        VIRT_REG(VIRT_GF_TTY_BASE + GF_TTY_PUT_CHAR) = *s++;
}

/* Prints the format string only: tests compare fixed strings. */
void
printf(char *fmt, ...)
{
    kputs(fmt);
}

void
panic(char *msg)
{
    kputs("panic: ");
    kputs(msg);
    kputs("\n");
    boot(rootdev, RB_HALT);
}

void
check(const char *what, int ok)
{
    if (! ok) {
        kputs("FAIL ");
        kputs(what);
        kputs("\n");
        failed = 1;
    }
}

/*
 * The vector table points into locore.S, which calls trap() and
 * fpfault_clear(); tests that link trap.c get the real ones.
 */
__attribute__((weak)) void
trap(void *frame)
{
    panic("unexpected trap");
}

__attribute__((weak)) void
fpfault_clear(u_char *state)
{
}

/* Print the result and stop QEMU, so the test ends without a timeout. */
void
report(const char *name)
{
    kputs(name);
    kputs(failed ? " FAILED\n" : " ok\n");
    boot(rootdev, RB_HALT);
}
