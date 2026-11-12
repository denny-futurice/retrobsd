/*
 * Support for the smoke tests that link the kernel's machdep.c: the kernel
 * symbols machdep.c needs, a console that writes straight to the Goldfish
 * TTY, and a check() helper.  Not a test itself.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kconfig.h>
#include <sys/reboot.h>
#include <machine/virt.h>
#include "ktest.h"

size_t physmem;
int boothowto, noproc;
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

/* Print the result and stop QEMU, so the test ends without a timeout. */
void
report(const char *name)
{
    kputs(name);
    kputs(failed ? " FAILED\n" : " ok\n");
    boot(rootdev, RB_HALT);
}
